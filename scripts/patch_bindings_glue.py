#!/usr/bin/env python3
"""Patch the generated C# bindings so GDExtension classes work at runtime.

The official mono glue has three extension-class gaps (verified against the
4.7-stable sources and godotengine/godot#75955):

  1. Generated/Constructors.cs only maps API_CORE classes
     (bindings_generator.cpp filters api_type != API_CORE), so wrapping a
     native extension-class object falls back to the nearest core base class
     (ClassDB.Instantiate("FWorld") came back as Godot.Node2D).
  2. csharp_script.cpp setup_csharp_script_binding deliberately skips
     GDExtension classes when choosing the wrapper type name ("...workaround
     to allow GDExtension classes to be used from C# so long as they're only
     used through base classes... See #75955"), so the managed wrapper
     factory is called with the base class name.
  3. For extension classes the engine stores the *concrete ancestor's*
     creation_func in ClassDB (class_db.cpp extension registration), while
     real construction goes through the GDExtension create_instance
     callback; the generated NativeCtor therefore builds the base class
     ('new FLevel()' built a Node2D).

This script rewrites, idempotently and deterministically:

  * Generated/GodotObjects/*.cs - for extension classes (identified by being
    absent from the core-class constructor map) the static NativeCtor is set
    to null, so ConstructAndInitialize falls back to ClassDB.Instantiate
    (which routes to the GDExtension create_instance).
  * Core/GodotObject.base.cs - ConstructAndInitialize instantiates through
    ClassDB when nativeCtor is null (adopting the raw pointer; the full
    managed tie keeps working as before).
  * Core/Bridge/ScriptManagerBridge.cs - CreateManagedForGodotObjectBinding
    re-wraps with the object's real class when a binding for it exists. The
    throw-away base wrapper is safe to drop: its IntPtr constructor only ties
    managed types that differ from the native type, never the case here.
  * Generated/Constructors.cs (GodotSharp and GodotSharpEditor) - adds
    marked wrapper entries for every generated class missing from the map.

NOTE: detection is entirely static / after-construction; never call ClassDB
bindings during GodotObject type initialization - singleton wrapper creation
re-enters binding creation and crashes the runtime with 0xC0000005.

Run it after --generate-mono-glue and before building the packages; it is
part of scripts/build_dotnet_bindings.ps1.
"""

import os
import re
import sys

PATCH_MARK = "[fragile-bindings patch]"
ENTRY_MARK = " // " + PATCH_MARK


def patch_godot_object_base(path):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    if PATCH_MARK in text:
        print("already patched: " + path)
        return

    old_init = """            if (NativePtr == IntPtr.Zero)
            {
                Debug.Assert(nativeCtor != null);

                // Need postinitialization.
                NativePtr = nativeCtor(godot_bool.True);"""
    new_init = """            if (NativePtr == IntPtr.Zero)
            {
                if (nativeCtor != null)
                {
                    // Need postinitialization.
                    NativePtr = nativeCtor(godot_bool.True);
                }
                else
                {
                    // [fragile-bindings patch] Extension classes: construct
                    // through ClassDB.Instantiate (routes to the GDExtension
                    // create_instance callback) and adopt the raw pointer.
                    NativePtr = VariantUtils.ConvertToGodotObjectPtr((godot_variant)ClassDB.Instantiate(nativeName).NativeVar);
                }"""
    if old_init not in text:
        raise SystemExit("pattern not found in " + path + ": ConstructAndInitialize body")
    text = text.replace(old_init, new_init)

    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(text)
    print("patched: " + path)


def patch_script_manager_bridge(path):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    if PATCH_MARK in text:
        print("already patched: " + path)
        return

    old = "                var instance = Constructors.Invoke(nativeTypeNameStr, godotObject);"
    new = """                var instance = Constructors.Invoke(nativeTypeNameStr, godotObject);

                // [fragile-bindings patch] The engine deliberately binds
                // GDExtension objects at their nearest engine base class
                // (csharp_script.cpp setup_csharp_script_binding skips
                // classinfo->gdextension; godotengine/godot#75955). Ask the
                // object for its real class and re-wrap when a binding for it
                // exists.
                string realTypeNameStr = instance.GetClass();
                if (realTypeNameStr != nativeTypeNameStr &&
                    Constructors.BuiltInMethodConstructors.ContainsKey(realTypeNameStr))
                {
                    instance = Constructors.Invoke(realTypeNameStr, godotObject);
                }"""
    if old not in text:
        raise SystemExit("pattern not found in " + path + ": CreateManagedForGodotObjectBinding body")
    text = text.replace(old, new)
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(text)
    print("patched: " + path)


def patch_constructors(ctor_dir, objects_dir):
    path = os.path.join(ctor_dir, "Constructors.cs")
    if not os.path.isfile(path):
        print("no Constructors.cs at " + path)
        return

    with open(path, encoding="utf-8") as handle:
        text = handle.read()

    official = set()
    for line in text.splitlines():
        match = re.search(r'\.Add\("([A-Za-z0-9_]+)"', line)
        if match and ENTRY_MARK not in line:
            official.add(match.group(1))

    missing = []
    for name in sorted(os.listdir(objects_dir)):
        if not name.endswith(".cs"):
            continue
        file_path = os.path.join(objects_dir, name)
        with open(file_path, encoding="utf-8") as handle:
            source = handle.read()
        class_match = re.search(r"public (?:abstract |partial )*class ([A-Za-z0-9_]+) :", source)
        native_match = re.search(r'StringName NativeName = "([A-Za-z0-9_]+)"', source)
        if not class_match or not native_match:
            continue
        class_name, native_name = class_match.group(1), native_match.group(1)
        if class_match.group(0).find("abstract") >= 0:
            continue
        if native_name in official:
            continue

        # Extension class: disable the bogus ancestor creation func so
        # ConstructAndInitialize falls back to ClassDB.Instantiate.
        old_ctor_line = "NativeCtor = ClassDB_get_constructor(NativeName);"
        if old_ctor_line in source:
            source = source.replace(old_ctor_line, "NativeCtor = null;" + ENTRY_MARK)
            with open(file_path, "w", encoding="utf-8", newline="") as handle:
                handle.write(source)
            print("disabled ancestor ctor: " + file_path)

        if ENTRY_MARK not in text.split(class_name)[0] and ('Add("' + native_name + '"') not in text:
            missing.append((class_name, native_name))

    if not missing:
        print("Constructors.cs already complete: " + path)
        return

    entries = "".join(
        "    BuiltInMethodConstructors.Add(\"" + native + "\", ptr => new " + cls + "(ptr));" + ENTRY_MARK + "\n"
        for cls, native in missing
    )
    lines = text.splitlines(keepends=True)
    insert_at = max(i for i, line in enumerate(lines) if ".Add(" in line) + 1
    lines.insert(insert_at, entries)
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write("".join(lines))
    print("patched: " + path + " (+" + str(len(missing)) + " entries: " + ", ".join(n for _, n in missing) + ")")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    root = os.path.abspath(sys.argv[1])
    glue = os.path.join(root, "modules", "mono", "glue", "GodotSharp")
    patch_godot_object_base(os.path.join(glue, "GodotSharp", "Core", "GodotObject.base.cs"))
    patch_script_manager_bridge(os.path.join(glue, "GodotSharp", "Core", "Bridge", "ScriptManagerBridge.cs"))
    patch_constructors(os.path.join(glue, "GodotSharp", "Generated"), os.path.join(glue, "GodotSharp", "Generated", "GodotObjects"))
    editor_gen = os.path.join(glue, "GodotSharpEditor", "Generated")
    if os.path.isdir(editor_gen):
        patch_constructors(editor_gen, os.path.join(editor_gen, "GodotObjects"))
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""Regression test for scripts/patch_bindings_glue.py.

Builds a synthetic GodotSharpEditor glue tree in .deps/ and checks the
extension-class patch lands where the generator actually puts things:

  * the factory file of an assembly is discovered, not hardcoded -
    GodotSharp writes Generated/Constructors.cs while GodotSharpEditor writes
    Generated/EditorConstructors.cs;
  * the emitted entry uses the factory dictionary identifier that file
    declares. GodotSharp says BuiltInMethodConstructors, GodotSharpEditor says
    builtInMethodConstructors (the parameter of AddEditorConstructors);
    hardcoding either spelling is a CS0103 in the other assembly;
  * the bogus ancestor NativeCtor is nulled for extension classes only;
  * a second run changes nothing.

Run with:  python scripts/test_patch_bindings_glue.py
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import patch_bindings_glue as pbg

SCRATCH = os.path.join(HERE, "..", ".deps", "patcher-test-scratch")
GEN = os.path.join(SCRATCH, "Generated")
OBJS = os.path.join(GEN, "GodotObjects")

EDITOR_CTORS = """namespace Godot;

internal static class EditorConstructors
{
    private static void AddEditorConstructors()
    {
        builtInMethodConstructors.Add("EditorPlugin", ptr => new EditorPlugin(ptr));
        builtInMethodConstructors.Add("Node2D", ptr => new Node2D(ptr));
    }
}
"""

CORE = """namespace Godot;

public partial class Node2D : CanvasItem
{
    private static readonly StringName NativeName = "Node2D";

    private static readonly unsafe delegate* unmanaged<godot_bool, IntPtr> NativeCtor = ClassDB_get_constructor(NativeName);
}
"""

EXTENSION = """namespace Godot;

public partial class FWorld : Node2D
{
    private static readonly StringName NativeName = "FWorld";

    private static readonly unsafe delegate* unmanaged<godot_bool, IntPtr> NativeCtor = ClassDB_get_constructor(NativeName);
}
"""

failures = []


def check(cond, msg):
    print(("  [ok]   " if cond else "  [FAIL] ") + msg)
    if not cond:
        failures.append(msg)


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def read(path):
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def build_fixture():
    shutil.rmtree(SCRATCH, ignore_errors=True)
    os.makedirs(OBJS, exist_ok=True)
    write(os.path.join(GEN, "EditorConstructors.cs"), EDITOR_CTORS)
    write(os.path.join(OBJS, "Node2D.cs"), CORE)
    write(os.path.join(OBJS, "FWorld.cs"), EXTENSION)


def main():
    build_fixture()

    print("1) find_constructors_file picks EditorConstructors.cs")
    found = pbg.find_constructors_file(GEN)
    check(found is not None and os.path.basename(found) == "EditorConstructors.cs", "found " + str(found))

    print("2) patch_constructors emits the extension class with the file's own identifier")
    pbg.patch_constructors(GEN, OBJS)
    text = read(os.path.join(GEN, "EditorConstructors.cs"))
    check('builtInMethodConstructors.Add("FWorld", ptr => new FWorld(ptr));' in text,
          "lowercase builtInMethodConstructors entry emitted")
    check("BuiltInMethodConstructors" not in text,
          "did not emit the GodotSharp-only BuiltInMethodConstructors identifier")
    core_lines = [ln for ln in text.splitlines() if 'Add("Node2D"' in ln]
    check(len(core_lines) == 1 and pbg.ENTRY_MARK not in core_lines[0], "core class Node2D left untouched")

    print("3) the bogus ancestor ctor is disabled only for the extension class")
    check("NativeCtor = null;" + pbg.ENTRY_MARK in read(os.path.join(OBJS, "FWorld.cs")),
          "FWorld.cs NativeCtor nulled")
    check("ClassDB_get_constructor" in read(os.path.join(OBJS, "Node2D.cs")),
          "core class Node2D keeps its real ctor")

    print("4) re-running is idempotent")
    before = text
    pbg.patch_constructors(GEN, OBJS)
    after = read(os.path.join(GEN, "EditorConstructors.cs"))
    check(before == after, "second run changed nothing")
    check(after.count('Add("FWorld"') == 1, "FWorld entry appears exactly once")

    print()
    if failures:
        print("FAILURES: " + str(len(failures)))
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())

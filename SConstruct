#!/usr/bin/env python
"""SConstruct for the 'fragile' GDExtension (Godot 4.7, godot-cpp v10).

Usage:
    scons                          # template_debug build, debug info ON
    scons target=template_release  # release/export build, debug info OFF
    scons debug_symbols=no         # force debug info off (any target)
    scons debug_symbols=yes        # force debug info on (any target)
    scons compiledb=yes            # also emit compile_commands.json (CLion / clangd)
    scons compiledb=yes compile_commands.json   # emit the DB without compiling
"""
import glob
import os
import sys

libname = "fragile"
projectdir = "project"

# godot-cpp is expected as a git submodule in this directory (see README).
if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print("ERROR: godot-cpp/ is missing or empty.")
    print("       Run: git submodule add https://github.com/godotengine/godot-cpp.git godot-cpp")
    print("       Then: git submodule update --init --recursive")
    sys.exit(1)

# --- debug symbols -------------------------------------------------------
# Debug information (a .pdb on MSVC, -g* on GCC/Clang) is ON by default so a
# development build is debuggable, and OFF for template_release: that is the
# variant an exported game ships, and it must not carry local debug info.
#
#   scons                          -> debug info ON   (template_debug)
#   scons target=template_release  -> debug info OFF  (export)
#   scons debug_symbols=no         -> force OFF
#   scons debug_symbols=yes        -> force ON
#
# godot-cpp reads debug_symbols straight off the SCons command line and
# defaults it to OFF (godotcpp.py: get_cmdline_bool("debug_symbols",
# env.dev_build)), so "auto" is resolved here and the plain yes/no is written
# back into ARGUMENTS before its SConscript runs - its BoolVariable would
# reject the value "auto" outright.
def resolve_debug_symbols(arguments):
    target = arguments.get("target", "template_debug").strip().lower()
    value = arguments.get("debug_symbols", "auto").strip().lower()
    if value in ("auto", ""):
        resolved = "no" if target == "template_release" else "yes"
    elif value in ("yes", "true", "on", "y", "1"):
        resolved = "yes"
    elif value in ("no", "false", "off", "n", "0"):
        resolved = "no"
    else:
        print("ERROR: debug_symbols must be auto, yes or no (got: " + value + ")")
        sys.exit(1)
    arguments["debug_symbols"] = resolved
    return resolved


debug_symbols = resolve_debug_symbols(ARGUMENTS)
print("debug symbols: " + ("on" if debug_symbols == "yes" else "off") + " (target=" + ARGUMENTS.get("target", "template_debug") + ")")

# Pin the Godot API version this extension builds against (godot-cpp v10
# supports 4.3 - 4.7). To target an exact engine build instead, use:
#     godot --dump-extension-api
#     scons custom_api_file=extension_api.json
env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# Headers live in include/ (mirroring the src/ subtree), implementation
# files in src/; includes are written root-relative, e.g.
# #include "fragile/framework/actor.h".
env.Append(CPPPATH=["include/"])

# SCons' Glob() does not recurse across directories; use Python's glob so
# every .cpp below src/ (e.g. src/fragile/framework/) is compiled. Headers
# under include/ are picked up through CPPPATH, not compiled themselves.
sources = sorted(glob.glob("src/**/*.cpp", recursive=True))

# ".dev" and ".universal" are not part of the .gdextension library keys.
suffix = env["suffix"].replace(".dev", "").replace(".universal", "")

# $SHLIBPREFIX / $SHLIBSUFFIX keep the naming toolchain-correct
# (no prefix on Windows, "lib" on Linux/macOS).
lib_filename = "{}{}{}{}".format(
    env.subst("$SHLIBPREFIX"), libname, suffix, env.subst("$SHLIBSUFFIX")
)

library = env.SharedLibrary(
    "{}/addons/fragile/bin/{}".format(projectdir, lib_filename),
    source=sources,
)

Default(library)

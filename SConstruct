#!/usr/bin/env python
"""SConstruct for the 'fragile' GDExtension (Godot 4.7, godot-cpp v10).

Usage:
    scons                          # template_debug build for the current platform
    scons target=template_release  # release build
    scons compiledb=yes            # also emit compile_commands.json (CLion / clangd)
    scons compiledb=yes compile_commands.json   # emit the DB without compiling
"""
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

# Pin the Godot API version this extension builds against (godot-cpp v10
# supports 4.3 - 4.7). To target an exact engine build instead, use:
#     godot --dump-extension-api
#     scons custom_api_file=extension_api.json
env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

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

#!/usr/bin/env python3
"""Build the fragile GodotSharp NuGet packages from a Godot source tree.

Thin wrapper around modules/mono/build_scripts/build_assemblies.py (from the
engine source tree) that runs exactly what the fragile bindings pipeline
needs, in the same order as its build_all():

  1. generate_sdk_package_versions()  pins package versions from version.py
     (scripts/build_dotnet_bindings.ps1 patches its 'status' first).
  2. build_godot_api()                builds GodotSharp + GodotSharpEditor +
     GodotPlugins and pushes the GodotSharp/GodotSharpEditor nupkgs.
  3. Godot.NET.Sdk.sln                builds the Godot.NET.Sdk +
     Godot.SourceGenerators nupkgs.

It intentionally skips build_all()'s GodotTools.sln step: GodotTools is the
editor-side C# tooling shipped inside the engine distribution
(GodotSharp/Tools), it produces no package this pipeline consumes, and it
pulls unrelated NuGet dependencies. Run build_assemblies.py directly when you
need those assemblies too.
"""

import argparse
import os
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--godot-source", required=True, help="engine source tree (matches the editor binary)")
    parser.add_argument("--godot-output-dir", required=True, help="where GodotSharp/Api/{Debug,Release} outputs go")
    parser.add_argument("--push-nupkgs-local", default="", help="local NuGet feed directory to copy nupkgs into")
    parser.add_argument("--precision", default="single", choices=["single", "double"])
    parser.add_argument("--no-deprecated", action="store_true")
    parser.add_argument("--werror", action="store_true")
    args = parser.parse_args()

    module_dir = os.path.abspath(os.path.join(args.godot_source, "modules", "mono"))
    build_script_dir = os.path.join(module_dir, "build_scripts")
    if not os.path.isfile(os.path.join(build_script_dir, "build_assemblies.py")):
        print(f"no build_assemblies.py under {build_script_dir}")
        return 1

    sys.path.insert(0, build_script_dir)
    import build_assemblies as ba

    # Force single-worker MSBuild without node reuse: the default multi-node
    # IPC uses named pipes, which the DSH Windows sandbox blocks (MSBuild then
    # exits 1 with no output at all). Harmless outside the sandbox.
    real_run_msbuild = ba.run_msbuild

    def run_msbuild_single(tools, sln, chdir_to, msbuild_args=None):
        return real_run_msbuild(tools, sln=sln, chdir_to=chdir_to, msbuild_args=list(msbuild_args or []) + ["/m:1", "/nr:false"])

    ba.run_msbuild = run_msbuild_single

    output_dir = os.path.abspath(args.godot_output_dir)
    push_nupkgs_local = os.path.abspath(args.push_nupkgs_local) if args.push_nupkgs_local else None

    tools = ba.find_any_msbuild_tool("")
    if tools is None:
        print("Unable to find MSBuild or dotnet CLI")
        return 1

    # 1. Version pinning (SdkPackageVersions.props + Common.Constants.cs).
    ba.generate_sdk_package_versions()

    # 2. GodotSharp / GodotSharpEditor / GodotPlugins (Debug + Release).
    exit_code = ba.build_godot_api(
        tools, module_dir, output_dir, push_nupkgs_local, args.precision, args.no_deprecated, args.werror
    )
    if exit_code != 0:
        return exit_code

    # 3. Godot.NET.Sdk + Godot.SourceGenerators packages. Built as the two
    # individual projects instead of Godot.NET.Sdk.sln: the solution also
    # contains the Sample and Tests projects, and Tests pulls an xunit/VS test
    # dependency closure that is irrelevant to packaging (and unreachable
    # from TLS-blocked sandboxes).
    msbuild_args = ["/restore", "/t:Build", "/p:Configuration=Release"]
    if push_nupkgs_local:
        msbuild_args += ["/p:ClearNuGetLocalCache=true", "/p:PushNuGetToLocalSource=" + push_nupkgs_local]
    if args.precision == "double":
        msbuild_args += ["/p:GodotFloat64=true"]
    if args.no_deprecated:
        msbuild_args += ["/p:GodotNoDeprecated=true"]
    sdk_dir = os.path.join(module_dir, "editor", "Godot.NET.Sdk")
    for project in [
        os.path.join(sdk_dir, "Godot.NET.Sdk", "Godot.NET.Sdk.csproj"),
        os.path.join(sdk_dir, "Godot.SourceGenerators", "Godot.SourceGenerators.csproj"),
    ]:
        exit_code = ba.run_msbuild(tools, sln=project, chdir_to=module_dir, msbuild_args=msbuild_args)
        if exit_code != 0:
            return exit_code
    return 0


if __name__ == "__main__":
    sys.exit(main())

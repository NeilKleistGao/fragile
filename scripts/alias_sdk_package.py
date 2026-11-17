#!/usr/bin/env python3
"""Create the Godot.NET.Sdk 4.7.0 alias package and evict stale Godot caches.

The stock Godot editor rewrites project/fragile.csproj's Sdk attribute to
exactly "Godot.NET.Sdk/4.7.0" every time it opens the project (GodotTools
ProjectUtils.EnsureGodotSdkIsUpToDate: any other value is rewritten and the
previous file is kept as fragile.csproj.old). Rather than fight the rewrite,
fragile.csproj uses that exact value and this alias package makes it resolve
to the custom bindings: the alias is a repack of the custom Godot.NET.Sdk with
only the nuspec <version> set to 4.7.0 - its SdkPackageVersions.props still
requests GodotSharp 4.7.0-<label>, so builds compile against the custom types.

The NuGet global-packages cache is keyed by id+version, so it keeps stale
extractions of both the alias and the rebuilt custom packages (same id+version
after every pipeline run!) and would win over the feed silently - the classic
cause of CS0246 on FWorld etc. The cache files are also read-only. This
script evicts the four Godot.* package folders from every NuGet cache it can
find; nuget.config's package source mapping keeps the stock packages from
being re-downloaded, so the next restore re-extracts the fresh ones.
"""

import argparse
import glob
import os
import re
import shutil
import stat
import sys
import zipfile

PACKAGE_IDS = [
    "godot.net.sdk",
    "godotsharp",
    "godotsharpeditor",
    "godot.sourcegenerators",
]


def make_alias(source_nupkg, alias_version):
    alias_nupkg = os.path.join(
        os.path.dirname(source_nupkg), "Godot.NET.Sdk." + alias_version + ".nupkg"
    )
    with zipfile.ZipFile(source_nupkg) as src:
        names = src.namelist()
        nuspec_name = next(n for n in names if n.endswith(".nuspec"))
        with zipfile.ZipFile(alias_nupkg, "w", zipfile.ZIP_DEFLATED) as dst:
            for name in names:
                data = src.read(name)
                if name == nuspec_name:
                    text = data.decode("utf-8")
                    pattern = re.compile(r"<version>[^<]+</version>")
                    text, count = pattern.subn(
                        lambda m: "<version>" + alias_version + "</version>", text
                    )
                    if count != 1:
                        raise SystemExit("could not rewrite <version> in " + name)
                    data = text.encode("utf-8")
                dst.writestr(name, data)
    print("alias package: " + alias_nupkg, flush=True)
    return alias_nupkg


def _force_remove(func, path, _exc):
    os.chmod(path, stat.S_IWRITE)
    func(path)


def evict_godot_caches(feed):
    roots = []
    env_root = os.environ.get("NUGET_PACKAGES")
    if env_root:
        roots.append(env_root)
    # The repo-local cache nuget.config's globalPackagesFolder points at.
    roots.append(os.path.join(os.path.dirname(os.path.abspath(feed)), "nuget-cache"))
    # The default user cache; irrelevant once globalPackagesFolder is set,
    # but worth cleaning when it is reachable.
    roots.append(os.path.join(os.path.expanduser("~"), ".nuget", "packages"))
    for root in roots:
        for package_id in PACKAGE_IDS:
            stale = os.path.join(root, package_id)
            if not os.path.isdir(stale):
                print("cache clean: " + stale, flush=True)
                continue
            try:
                shutil.rmtree(stale, onerror=_force_remove)
                print("evicted cache: " + stale, flush=True)
            except OSError as error:
                print("WARNING: could not evict " + stale + ": " + str(error), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--feed", required=True, help="local NuGet feed directory")
    parser.add_argument("--alias-version", default="4.7.0")
    args = parser.parse_args()

    candidates = [
        p
        for p in glob.glob(os.path.join(args.feed, "Godot.NET.Sdk.*.nupkg"))
        if not os.path.basename(p).startswith("Godot.NET.Sdk." + args.alias_version + ".")
    ]
    if not candidates:
        raise SystemExit("no versioned Godot.NET.Sdk nupkg found in " + args.feed)
    source = max(candidates, key=os.path.getmtime)
    print("source package: " + source, flush=True)
    make_alias(source, args.alias_version)
    evict_godot_caches(args.feed)
    return 0


if __name__ == "__main__":
    sys.exit(main())

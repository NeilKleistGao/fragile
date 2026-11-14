#!/usr/bin/env python3
"""Offline helper: download NuGet packages (plus dependency closure) into a
local folder feed without dotnet restore.

Sandboxed agent runs cannot do TLS via schannel, so dotnet restore cannot
reach nuget.org (NU1301) and fails for any package the local feeds do not
provide. Python's OpenSSL stack is unaffected: this script downloads the
.nupkg files from the nuget.org v3 flat container into a plain folder that
NuGet accepts as a local source (e.g. .deps/nupkgs, already wired up in
nuget.config), so restore then resolves fully offline.

Usage:
  python scripts/fetch_nuget_packages.py --out <folder-feed> id:version [id:version ...]

Idempotent: packages already present in the folder are skipped.
"""

import os
import sys
import urllib.request
import xml.etree.ElementTree as ET
import zipfile

FLAT_CONTAINER = "https://api.nuget.org/v3-flatcontainer/{id}/{version}/{id}.{version}.nupkg"


def lower_bound(version_range):
    value = version_range.strip()
    for char in "[()]":
        value = value.replace(char, "")
    return value.split(",")[0].strip()


def parse_dependencies(nuspec):
    root = ET.fromstring(nuspec)
    deps = []
    for node in root.iter():
        if node.tag.split("}")[-1] != "dependency":
            continue
        pkg_id = node.attrib.get("id", "")
        version = lower_bound(node.attrib.get("version", ""))
        if pkg_id and version:
            deps.append((pkg_id, version))
    return deps


def install(pkg_id, version, out_dir, seen):
    key = (pkg_id.lower(), version)
    if key in seen:
        return
    seen.add(key)

    nupkg_name = pkg_id.lower() + "." + version + ".nupkg"
    nupkg_path = os.path.join(out_dir, nupkg_name)
    if os.path.isfile(nupkg_path):
        print("have    " + pkg_id + " " + version)
        return

    url = FLAT_CONTAINER.format(id=pkg_id.lower(), version=version.lower())
    print("fetch   " + pkg_id + " " + version + " <- " + url, flush=True)
    with urllib.request.urlopen(url, timeout=120) as resp:
        nupkg = resp.read()

    os.makedirs(out_dir, exist_ok=True)
    with open(nupkg_path, "wb") as handle:
        handle.write(nupkg)

    nuspec = b""
    with zipfile.ZipFile(nupkg_path) as archive:
        for name in archive.namelist():
            if name.lower().endswith(".nuspec"):
                nuspec = archive.read(name)
                break

    for dep_id, dep_version in parse_dependencies(nuspec):
        install(dep_id, dep_version, out_dir, seen)


def main():
    args = sys.argv[1:]
    out_dir = "."
    if args and args[0] == "--out":
        if len(args) < 2:
            print("--out needs a folder")
            return 2
        out_dir = args[1]
        args = args[2:]
    if not args:
        print(__doc__)
        return 2

    seen = set()
    for spec in args:
        pkg_id, sep, version = spec.partition(":")
        if not sep or not version:
            print("bad spec (want id:version): " + spec)
            return 2
        install(pkg_id, version, out_dir, seen)
    print("done; " + str(len(seen)) + " package(s) processed; folder feed: " + os.path.abspath(out_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())

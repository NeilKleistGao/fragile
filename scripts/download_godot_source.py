#!/usr/bin/env python3
"""Download the Godot engine source tree that matches the editor binary.

Used by scripts/build_dotnet_bindings.ps1 (see AGENTS.md, "C# / .NET bindings
pipeline"). On this network github.com is DNS-poisoned and sandboxed processes
cannot do TLS via schannel (curl/git/Invoke-WebRequest all fail), but Python's
OpenSSL stack works, so we fetch the tag zip through the ghproxy mirror with
plain urllib. On normal networks the direct GitHub URL is tried as fallback.
"""

import argparse
import os
import re
import shutil
import sys
import urllib.request
import zipfile

MIRRORS = [
    "https://ghproxy.net/https://github.com/godotengine/godot/archive/refs/tags/{tag}.zip",
    "https://github.com/godotengine/godot/archive/refs/tags/{tag}.zip",
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", default="4.7-stable", help="engine source tag (default: 4.7-stable)")
    parser.add_argument("--dest", required=True, help="target tree directory, e.g. .deps/godot-4.7-stable")
    args = parser.parse_args()

    dest = os.path.abspath(args.dest)
    # A complete tree has the GodotSharp solution; a partially probed tree may
    # contain version.py but nothing else.
    if os.path.isfile(os.path.join(dest, "modules", "mono", "glue", "GodotSharp", "GodotSharp.sln")):
        print(f"source tree already present: {dest}")
        return 0

    parent = os.path.dirname(dest) or "."
    os.makedirs(parent, exist_ok=True)
    zip_path = dest + ".download.zip"
    extract_dir = dest + ".extract"

    for mirror in MIRRORS:
        url = mirror.format(tag=args.tag)
        print(f"downloading {url} ...", flush=True)
        try:
            with urllib.request.urlopen(url, timeout=120) as resp, open(zip_path, "wb") as out:
                while True:
                    chunk = resp.read(1 << 20)
                    if not chunk:
                        break
                    out.write(chunk)
            print(f"downloaded {os.path.getsize(zip_path)} bytes", flush=True)
            break
        except Exception as exc:  # noqa: BLE001 - report and try next mirror
            print(f"failed: {exc}", flush=True)
            if os.path.exists(zip_path):
                os.remove(zip_path)
    else:
        print("all mirrors failed")
        return 1

    print("extracting ...", flush=True)
    if os.path.isdir(extract_dir):
        shutil.rmtree(extract_dir)
    with zipfile.ZipFile(zip_path) as archive:
        archive.extractall(extract_dir)
    entries = os.listdir(extract_dir)
    if len(entries) != 1:
        print(f"unexpected archive layout: {entries}")
        return 1
    if os.path.isdir(dest):
        shutil.rmtree(dest)
    os.rename(os.path.join(extract_dir, entries[0]), dest)
    shutil.rmtree(extract_dir, ignore_errors=True)
    os.remove(zip_path)

    with open(os.path.join(dest, "version.py"), encoding="utf-8") as handle:
        version_py = handle.read()
    fields = {k: v.strip().strip('"') for k, v in re.findall(r"^(major|minor|patch|status) = (.*)$", version_py, re.M)}
    print(f"source tree ready: {dest}")
    print("version.py says: {}.{}.{} {}".format(fields.get("major"), fields.get("minor"), fields.get("patch"), fields.get("status")))
    return 0


if __name__ == "__main__":
    sys.exit(main())

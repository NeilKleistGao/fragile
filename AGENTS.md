# AGENTS.md — fragile GDExtension

Guidance for agents (and humans) working on this repository: what the project is,
how to build and verify it, and the pitfalls already discovered on the way.
Read this before changing build scripts or running engine tests.

## Project overview

A bare GDExtension plugin named `fragile` for **Godot 4.7** (C++ via
**godot-cpp v10**). Layout:

```
SConstruct              # build script, pins api_version="4.7"
godot-cpp/              # C++ bindings (see "godot-cpp provenance" below)
src/                    # ALL C++ sources of the extension live here
project/                # Godot test project (project.godot, main.tscn)
project/addons/fragile/bin/  # fragile.gdextension + built shared libraries
```

Repo rule: every C++ file of the extension goes into `src/`. Nothing else.

## Build & test workflow

```sh
# Build (template_debug; output lands in project/addons/fragile/bin/)
scons

# Release build
scons target=template_release

# compile_commands.json for CLion / clangd (built into godot-cpp v10)
scons compiledb=yes                     # build + emit DB
scons compiledb=yes compile_commands.json   # emit DB only

# One-time import (see pitfall #1), then smoke test
"C:\Program Files\Godot\Godot_v4.7-stable_mono_win64_console.exe" --headless --editor --quit-after 60 --path project
"C:\Program Files\Godot\Godot_v4.7-stable_mono_win64_console.exe" --headless --path project --quit-after 60
```

A clean smoke test exits **0** with only benign noise (see "Environment
pitfalls"). Anything resembling the failure signatures in the checklist below
means the extension did not load.

## Engine pitfalls (Godot 4.7 / godot-cpp v10)

### 1. `extension_list.cfg` must exist or the engine never sees the extension
The engine loads GDExtensions listed in `project/.godot/extension_list.cfg`,
which is generated **when the editor first imports the project**. Until that
file exists, headless runs fail with a misleading class error even though the
DLL is correct and correctly named:

```
ERROR: Cannot get class 'Fragile'.
WARNING: Node Main of type Fragile cannot be created. A placeholder will be created instead.
```

Fix: run the one-time headless editor import (command above). Afterwards
`project/.godot/extension_list.cfg` contains
`res://addons/fragile/bin/fragile.gdextension` and headless runs work. **When a test reports "Cannot get class", check this
file first — it is the most likely cause, not the C++ code.**

### 2. godot-cpp v10 is versioned independently from Godot
There is no `godot-4.7-stable` tag (tagging stopped after `godot-4.5-stable`;
releases are now `10.0.0-stable`, `10.0.0-rc2`, ...). The target engine is
selected via SCons options instead:

- `api_version` — pinned in this repo's `SConstruct` as
  `SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})`.
  Allowed values in v10: `"4.3"` … `"4.7"`.
- `custom_api_file=extension_api.json` — for an exact engine build, dump with
  `godot --dump-extension-api` (takes precedence over `api_version`).

Bundled API JSONs live in `godot-cpp/gdextension/extension_api-4-7.json` etc.
(note the dash-separated version in the filename); the interface header is
*generated* at build time into `godot-cpp/gen/include/gdextension_interface.h`,
so do not expect it in the source tree before the first build.

### 3. Library naming is a hard contract between SConstruct and .gdextension
The build output name is
`$SHLIBPREFIX + fragile + env["suffix"] + $SHLIBSUFFIX`, e.g.
`fragile.windows.template_debug.x86_64.dll` / `libfragile.linux.template_debug.x86_64.so`.
The `SConstruct` strips `.dev` and `.universal` from the suffix because those
are not part of the `.gdextension` keys. If you rename the library or change
targets, update `project/addons/fragile/bin/fragile.gdextension` in the same
commit or the
engine silently falls back to a placeholder node.

### 4. Entry symbol contract
`entry_symbol = "fragile_library_init"` in `fragile.gdextension` must equal the
`extern "C"` exported function in `src/register_types.cpp`. Verify a suspect
DLL with `dumpbin /exports` — the export list should contain exactly
`fragile_library_init`.

### 5. Version pairing
`compatibility_minimum = "4.7"` pairs with `api_version = "4.7"`. Extensions
built against an older API run on newer Godot, not vice versa — do not raise
`compatibility_minimum` casually; it gates loading on older engines.

### 6. `reloadable = true` is debug-only
Hot reload of a recompiled extension works only for `template_debug` builds.
Release builds require an editor restart. Keep `reloadable = true` while
developing.

### 7. Official docs moved in 4.6/4.7
Old URLs 404. Current locations:
- GDExtension system: `docs.godotengine.org/en/stable/engine_details/engine_api/gdextension/`
  (`what_is_gdextension`, `gdextension_file`, `gdextension_c_example`, ...)
- godot-cpp (C++) docs: `docs.godotengine.org/en/stable/tutorials/scripting/cpp/`
  (`gdextension_cpp_example` = "Getting started", `build_system`, ...)
- The `.gdextension` file reference is authoritative for key names and
  semantics: `compatibility_minimum`, `compatibility_maximum`, `reloadable`,
  `android_aar_plugin`, per-platform `[libraries]` keys.

## Environment pitfalls (this machine / sandboxed agent runs)

- **GitHub is DNS-poisoned** on this network. `github.com` and
  `raw.githubusercontent.com` are unreachable; `ghproxy.net` mirrors work for
  both raw files and zip downloads
  (`https://ghproxy.net/https://github.com/<org>/<repo>/archive/refs/tags/<tag>.zip`).
- **Sandboxed processes cannot do TLS** (`SEC_E_NO_CREDENTIALS` from schannel;
  curl, git, and `Invoke-WebRequest` all fail). Downloading dependencies needs
  a one-time elevated run. Plain `git clone` through the proxy also fails with
  this error even with `http.sslBackend=openssl` — download and extract a zip
  instead.
- **Godot crashes with signal 11 in the sandbox** if it cannot write
  `user://logs` (which resolves under `%APPDATA%\Godot`). This is a sandbox
  artifact, not a bug. Workaround for headless tests without elevation:
  redirect `APPDATA` at the process level
  (`$env:APPDATA = <workspace>\.godot-appdata` before launching the engine).
- **`ERROR: Failed to read the root certificate store`** is sandbox noise and
  appears on every run; ignore it. Likewise `.NET: Failed to load project
  assembly` is expected when using the mono editor build on this C++-only
  project.
- **PowerShell pipeline truncation mimics failure**: piping a native command
  into `Select-Object -First N` closes the pipe early and the process exits 1
  with a broken-pipe error even when the real work succeeded (this made a
  correct `scons -n` dry run look failed). Capture output first
  (`| Out-String`), then slice.
- **The `--quit-after N` flag counts frames, not seconds.**
- SCons on this machine: `%APPDATA%\Python\Python311\Scripts\scons.exe`
  (v4.4). MSVC is Visual Studio 18 Community, found via vswhere
  (`%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe`).
  The machine has 16 cores; godot-cpp auto-parallelizes to 15.

## CI (.github/workflows/ci.yml)

Two jobs: `lint` (clang-format on `src/`) and `build` (4-way matrix: Windows
debug/release, Linux debug, macOS debug), with a headless engine smoke test on
the Windows debug leg.

- **Language standard**: the repo ships `godot-cpp`'s official `.clang-format`
  (verbatim copy). CI pins `clang-format==22.1.3` (PyPI) to match the local
  VS 2026 LLVM toolchain exactly — unpinned clang-format versions reformat
  differently and would desync local runs from CI. Check locally with
  `<VS>\VC\Tools\Llvm\x64\bin\clang-format.exe --dry-run --Werror src\*.h src\*.cpp`.
- **YAML pitfall**: a single-line `run:` must not start with `&` — YAML parses
  it as an anchor and the workflow silently compiles to **zero jobs**. Always
  use `run: |` blocks when a command may start with `&` (PowerShell call
  operator). This hit real workflows before (see also the GitHub Actions note
  in the editor-plugin skill notes).
- **godot-cpp in CI**: the dev machine carries `godot-cpp/` as a plain
  downloaded (gitignored) directory; the workflow clones
  `10.0.0-stable` when checkout did not provide it, so the repo works with or
  without a submodule.
- The smoke-test legs replicate AGENTS.md pitfall #1 on purpose (editor import
  first, then the run) and grep the failure signatures from the checklist —
  edit them together.
- CI downloads Godot from `godotengine/godot-builds` releases; the Windows
  console exe lives **inside** `Godot_v4.7-stable_win64.exe.zip` (there is no
  separate console zip asset).

## Verification checklist (run after any build-script or registration change)

1. `scons` exits 0 and the expected library appears in
   `project/addons/fragile/bin/` with the
   exact name referenced by `fragile.gdextension`.
2. `dumpbin /exports <dll>` lists `fragile_library_init`.
3. Headless editor import succeeds ("validating GDExtension" in output) and
   `project/.godot/extension_list.cfg` lists
   `res://addons/fragile/bin/fragile.gdextension`.
4. Headless run exits 0 and loads `main.tscn` whose root node is a `Fragile`
   instance — this exercises class registration end to end.

Failure signatures:

| Symptom | Meaning |
|---|---|
| `Cannot get class 'Fragile'` + placeholder warning | Extension not loaded: missing `extension_list.cfg`, wrong lib path/name in `.gdextension`, or missing entry symbol |
| `Can't open dynamic library` | Lib path in `[libraries]` does not match build output |
| Signal 11 right at startup, `Could not create directory: 'user://logs'` | Sandbox blocked `%APPDATA%` writes — use the APPDATA redirect, not a code fix |
| exit 1 from `scons` with broken pipe text | Likely the PowerShell `-First` truncation artifact; re-run capturing full output |

## Conventions

- All extension C++ under `src/`; registration code in `register_types.cpp`;
  one class per header/source pair; `GDCLASS` macro + `_bind_methods()`.
- Keep `SConstruct`, `src/register_types.cpp` (entry symbol) and
  `project/addons/fragile/bin/fragile.gdextension` consistent — they form three
   ends of one
  contract (build name, export name, loader config).
- Build outputs (`project/addons/fragile/bin/*.dll`, `*.os`, `.sconsign.dblite`,
  `compile_commands.json`) and smoke-test residue (`.godot-appdata/`,
  `*.log`) are git-ignored; do not commit them.

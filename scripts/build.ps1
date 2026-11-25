<#
.SYNOPSIS
  One-click build for the fragile GDExtension: compile and verify.

.DESCRIPTION
  Produces a usable extension in project/addons/fragile/bin/ - or fails loudly
  with the reason. Nothing is assumed; every contract end is checked.

    0. preflight   Resolve scons + the Godot editor. Refuse to build while a
                   Godot process holds the output DLL: that is exactly what
                   silently produced the stale '~fragile.*.dll' shadow copies
                   in bin/ and left the editor running an old extension, i.e.
                   'cannot launch from the editor at all'.
    1. build       scons <target>. Full output goes to .deps/scons-build.log;
                   it is captured first and never piped through
                   Select-Object -First (that truncation artifact makes a good
                   build look failed - see AGENTS.md).
    2. verify      The DLL exists, was actually REPLACED this run (timestamp),
                   no shadow copies were left behind, its name matches the
                   [libraries] key in fragile.gdextension, it exports the
                   entry_symbol that same file declares, and its debug
                   symbols match the -DebugInfo setting.
    3. import      One-time headless editor import so
                   project/.godot/extension_list.cfg exists. Without it the
                   engine never loads the extension and reports the misleading
                   'Cannot get class' (AGENTS.md pitfall #1).

  Exits 0 only when all of the above passed. Safe and cheap to re-run.

.PARAMETER Target
  template_debug (default; hot-reloadable) or template_release.

.PARAMETER DebugInfo
  Debug information for the built library (a .pdb on MSVC, -g* elsewhere).
    auto (default) - ON for template_debug, OFF for template_release. The
                     release variant is what an exported game ships, so it
                     must not carry local debug info.
    on             - always produce debug symbols.
    off            - never produce debug symbols.
  Maps to scons debug_symbols=yes|no.

.PARAMETER Bindings
  Also regenerate the .NET bindings via scripts/build_dotnet_bindings.ps1.
  Do this whenever _bind_methods() signatures changed - the generated wrappers
  carry method-compatibility hashes and stale ones fail at runtime with a
  method-bind mismatch.

  build.cmd turns this on by default (together with -InstallApiTo), because
  forgetting it is what leaves the editor showing methods you already deleted.

.PARAMETER SkipBindings
  Suppress the -Bindings step. build.cmd always asks for bindings, so this is
  the opt-out when you only want the native library rebuilt. (PowerShell
  rejects a parameter given twice, so -Bindings:$false cannot override it.)

.PARAMETER GodotExe
  Godot 4.7 .NET editor used for the import check and forwarded to the .NET
  bindings pipeline. Defaults to the portable C# editor under
  .deps/godot-4.7-fragile when present, otherwise to the stock installed
  editor. Pass a different editor here to build/import against it.

.PARAMETER InstallApiTo
  With -Bindings: editor directory whose GodotSharp/Api gets the freshly built
  assemblies. Editor-mode runs load bindings from <exe dir>/GodotSharp/Api only,
  so point this at the editor you actually launch (e.g. .deps/godot-4.7-fragile).

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1

  The script is unsigned; a bare './scripts/build.ps1' fails under restrictive
  execution policies (SecurityError), so use -ExecutionPolicy Bypass.
#>
[CmdletBinding()]
param(
    [ValidateSet('template_debug', 'template_release')]
    [string] $Target = 'template_debug',

    [ValidateSet('auto', 'on', 'off')]
    [string] $DebugInfo = 'auto',

    [string] $GodotExe = '',

    [switch] $SkipImport,
    [switch] $AllowRunningEditor,

    [switch] $Bindings,
    [switch] $SkipBindings,
    [string] $InstallApiTo = ''
)

$ErrorActionPreference = 'Stop'
# $PSScriptRoot is empty inside param() defaults on Windows PowerShell 5.1,
# so repo-relative paths are resolved here in the script body.
$repoRoot   = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$projectDir = Join-Path $repoRoot 'project'
$binDir     = Join-Path $projectDir 'addons\fragile\bin'
$gdextPath  = Join-Path $binDir 'fragile.gdextension'
$logDir     = Join-Path $repoRoot '.deps'

$buildStart = Get-Date

# Prefer the portable editor that carries the custom GodotSharp/Api (built by
# scripts/build_dotnet_bindings.ps1). Editor-mode runs load their bindings from
# <exe dir>/GodotSharp/Api, so the stock Program Files editor would wrap
# fragile classes at their engine base type.
if (-not $GodotExe) {
    $portableEditorExe = Join-Path $repoRoot '.deps\godot-4.7-fragile\Godot_v4.7-stable_mono_win64_console.exe'
    if (Test-Path -LiteralPath $portableEditorExe) {
        $GodotExe = $portableEditorExe
    } else {
        $GodotExe = 'C:\Program Files\Godot\Godot_v4.7-stable_mono_win64_console.exe'
    }
}

function Write-Stage([string] $Message) { Write-Host ''; Write-Host ('== ' + $Message) -ForegroundColor Cyan }
function Write-Ok([string] $Message)    { Write-Host ('   [ok]   ' + $Message) -ForegroundColor Green }
function Write-Info([string] $Message)  { Write-Host ('   [--]   ' + $Message) }
function Write-Note([string] $Message)  { Write-Host ('   [note] ' + $Message) -ForegroundColor Yellow }
function Fail([string] $Message) {
    Write-Host ('   [FAIL] ' + $Message) -ForegroundColor Red
    Write-Host ''
    Write-Host 'Build aborted: the extension is NOT usable.' -ForegroundColor Red
    exit 1
}

function Invoke-Captured {
    # Runs a native command and returns its exit code and merged output WITHOUT
    # PowerShell turning stderr into an error record. Capture into a variable
    # first - piping a native command straight out makes a good build look
    # failed (AGENTS.md: PowerShell pipeline truncation mimics failure).
    param([string] $FilePath, [string[]] $ArgumentList = @())
    # A native command writing to stderr becomes an ErrorRecord on the
    # pipeline once 2>&1 is applied; under $ErrorActionPreference = 'Stop'
    # that aborts the script even when the command succeeded. Relax it only
    # for the duration of the native call.
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $raw = & $FilePath @ArgumentList 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prevEAP
    }
    $out = ($raw | ForEach-Object { $_.ToString() }) -join [string][char]10
    return [pscustomobject]@{ ExitCode = $code; Output = $out }
}

# --------------------------------------------------------------------------
Write-Stage '0/3 preflight'
# --------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $gdextPath)) {
    Fail ('missing ' + $gdextPath + ' - is this a fragile checkout?')
}
if (-not (Test-Path -LiteralPath $GodotExe)) {
    Fail ('Godot editor binary not found: ' + $GodotExe)
}

$scons = Get-Command scons.exe -ErrorAction SilentlyContinue
if (-not $scons) {
    $candidate = Join-Path $env:APPDATA 'Python\Python311\Scripts\scons.exe'
    if (Test-Path -LiteralPath $candidate) { $scons = Get-Item -LiteralPath $candidate }
}
if (-not $scons) {
    Fail 'scons.exe not found on PATH or under %APPDATA%\Python\Python311\Scripts. Install with: pip install scons'
}
# Get-Command yields CommandInfo (-> .Source); Get-Item yields FileInfo
# (-> .FullName). Normalise to a plain path so '&' always gets a string.
$sconsPath = if ($scons -is [System.Management.Automation.CommandInfo]) { $scons.Source } else { $scons.FullName }
Write-Info ('scons:  ' + $sconsPath)
Write-Info ('godot:  ' + $GodotExe)
Write-Info ('target: ' + $Target)

# --- debug information ----------------------------------------------------
# On by default so a development build is debuggable; off for template_release,
# which is the variant an exported game ships - it must not carry local debug
# info. -DebugInfo on|off forces it either way; 'auto' picks from the target.
$debugInfoValue = $DebugInfo.ToLowerInvariant()
if ($debugInfoValue -eq 'auto') {
    $debugSymbols = if ($Target -eq 'template_release') { 'no' } else { 'yes' }
} elseif ($debugInfoValue -eq 'on') {
    $debugSymbols = 'yes'
} else {
    $debugSymbols = 'no'
}
$debugLabel = if ($debugSymbols -eq 'yes') { 'on' } else { 'off' }
Write-Info ('debug:  ' + $debugLabel + '  (-DebugInfo ' + $DebugInfo + ')')

# --- the DLL-lock trap ----------------------------------------------------
# A Godot editor that has the extension loaded keeps the DLL open. The link
# step then cannot replace it: Windows parks the old image as
# '~fragile.*.dll' / '~fragile.*.dll~RF*.TMP' and the editor keeps running the
# previous build. Catch that BEFORE compiling.
$gdextText = Get-Content -LiteralPath $gdextPath -Raw
$flavour   = if ($Target -eq 'template_release') { 'release' } else { 'debug' }
$libMatch  = [regex]::Match($gdextText, '(?m)^\s*windows\.' + $flavour + '\.[A-Za-z0-9_]+\s*=\s*"(?<p>[^"]+)"')
if (-not $libMatch.Success) {
    Fail ('fragile.gdextension has no [libraries] entry for windows.' + $flavour + '.*')
}
$libFileName = [System.IO.Path]::GetFileName($libMatch.Groups['p'].Value)
$libPath     = Join-Path $binDir $libFileName
Write-Info ('lib:    ' + $libFileName)

if (Test-Path -LiteralPath $libPath) {
    $locked = $false
    try {
        $fs = [System.IO.File]::Open($libPath, 'Open', 'ReadWrite', 'None')
        $fs.Dispose()
    } catch {
        $locked = $true
    }
    if ($locked) {
        $holders = @(Get-Process -Name 'Godot*' -ErrorAction SilentlyContinue)
        if ($holders -and -not $AllowRunningEditor) {
            Fail ('the output DLL is locked, most likely by a running Godot editor (pid ' + (($holders | ForEach-Object { $_.Id }) -join ', ') + '). Close the editor and re-run, or pass -AllowRunningEditor.')
        }
        Write-Note 'output DLL is locked; continuing because -AllowRunningEditor was passed.'
    }
}

# --- clean Windows pending-rename leftovers only --------------------------
# NEVER delete '<name>.dll' / '<name>_0.pdb' starting with '~'. In editor mode
# Godot copies the GDExtension library to '~<name>.dll' ON PURPOSE
# (core/extension -> platform/windows/os_windows.cpp open_dynamic_library,
# generate_temp_files = Engine::is_editor_hint()) so the original stays free
# for the compiler, and copy_and_rename_pdb() mirrors the PDB next to it.
# Those are normal editor artifacts, created at editor start and removed at
# editor close. Deleting them breaks a running editor and makes the next
# engine run fail with "Error copying library" / "Can't open GDExtension
# dynamic library" - which is exactly what hid the extension from
# --generate-mono-glue.
# What IS junk is '<name>~RFxxxxxxxx.TMP': a MoveFileEx pending rename that
# never completed because the destination was locked.
$pending0 = @(Get-ChildItem -LiteralPath $binDir -Force -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -like '*~RF*.TMP' })
foreach ($old in $pending0) {
    Write-Note ('removing Windows pending-rename leftover: ' + $old.Name + ' (' + $old.LastWriteTime + ')')
    Remove-Item -LiteralPath $old.FullName -Force
}

# --------------------------------------------------------------------------
Write-Stage '1/3 build'
# --------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$sconsLog = Join-Path $logDir 'scons-build.log'
$buildArgs = @()
if ($Target -ne 'template_debug') { $buildArgs += ('target=' + $Target) }
$buildArgs += ('debug_symbols=' + $debugSymbols)
Write-Info ('scons ' + ($buildArgs -join ' '))
$build = Invoke-Captured -FilePath $sconsPath -ArgumentList $buildArgs
Set-Content -LiteralPath $sconsLog -Value $build.Output -Encoding utf8
if ($build.ExitCode -ne 0) {
    Write-Host $build.Output
    Fail ('scons failed with exit code ' + $build.ExitCode + ' - full log: ' + $sconsLog)
}
Write-Ok ('scons exit 0 (log: ' + $sconsLog + ')')
# Remember whether scons actually relinked: on an up-to-date tree it does
# nothing, and the "was the DLL replaced?" check below must not treat that as
# a locked-file failure.
$didLink = ($build.Output -match [regex]::Escape('Linking Shared Library'))

# --------------------------------------------------------------------------
Write-Stage '2/3 verify the artefact'
# --------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $libPath)) {
    Fail ('expected library was not produced: ' + $libPath)
}
Write-Ok ('library present: ' + $libFileName)

if ($didLink) {
    # A locked DLL makes the linker park the old image and NOT replace the
    # file. If the timestamp predates this run, the build did not land.
    $item = Get-Item -LiteralPath $libPath
    if ($item.LastWriteTime -lt $buildStart) {
        Fail ('the library was NOT replaced this run (last write ' + $item.LastWriteTime + ', build started ' + $buildStart + '). A running editor is holding it.')
    }
    Write-Ok ('library was replaced this run (' + $item.LastWriteTime + ')')
} else {
    Write-Ok ('nothing to relink; the existing library is already current (' + (Get-Item -LiteralPath $libPath).LastWriteTime + ')')
}

# Careful what counts as evidence here. MSVC's linker renames the previous
# output aside as '<name>.dll' / '<name>_0.pdb' as part of a SUCCESSFUL relink,
# so a plain '~...' byproduct proves nothing. The unambiguous sign that the
# output could not be replaced is a Windows pending-rename leftover,
# '<name>~RFxxxxxxxx.TMP' (MoveFileEx deferred); the DLL timestamp check above
# is what actually proves the new build landed.
$pending = @(Get-ChildItem -LiteralPath $binDir -Force -ErrorAction SilentlyContinue |
             Where-Object { $_.Name -like '*~RF*.TMP' })
if ($pending) {
    Fail ('Windows left pending-rename leftovers, so the output could not be replaced: ' + (($pending | ForEach-Object { $_.Name }) -join ', '))
}
$godotCopies = @(Get-ChildItem -LiteralPath $binDir -Force -ErrorAction SilentlyContinue |
                 Where-Object { $_.Name -like '~*' })
foreach ($copy in $godotCopies) {
    Write-Note ('Godot editor working copy present: ' + $copy.Name + ' (an editor has/had this extension loaded; Godot manages this file)')
}

# entry_symbol from fragile.gdextension must be exported by the DLL
$entryMatch = [regex]::Match($gdextText, '(?m)^\s*entry_symbol\s*=\s*"(?<s>[^"]+)"')
if (-not $entryMatch.Success) {
    Fail 'fragile.gdextension declares no entry_symbol.'
}
$entrySymbol = $entryMatch.Groups['s'].Value
$exportsOk = $false
$dumpbinPath = $null
$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($dumpbin) {
    $dumpbinPath = $dumpbin.Source
} else {
    $pf86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $vswhere = Join-Path $pf86 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $vsPath = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>&1 | Out-String).Trim()
        if ($vsPath) {
            $hit = Get-ChildItem -Path (Join-Path $vsPath 'VC\Tools\MSVC') -Recurse -Filter 'dumpbin.exe' -ErrorAction SilentlyContinue |
                   Where-Object { $_.DirectoryName -match 'Hostx64\\x64$' } | Select-Object -First 1
            if ($hit) { $dumpbinPath = $hit.FullName }
        }
    }
}
if ($dumpbinPath) {
    $dump = Invoke-Captured -FilePath $dumpbinPath -ArgumentList @('/nologo', '/exports', $libPath)
    $exportsOk = ($dump.Output -match [regex]::Escape($entrySymbol))
    Write-Info 'export table checked with dumpbin'
} else {
    # dumpbin unavailable: fall back to scanning the export-name strings.
    $ascii = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($libPath))
    $exportsOk = $ascii.Contains($entrySymbol)
    Write-Note 'dumpbin.exe not found; checked the export name with a byte scan instead.'
}
if (-not $exportsOk) {
    Fail ('the DLL does not export "' + $entrySymbol + '" - entry_symbol in fragile.gdextension and the extern "C" export in src/register_types.cpp disagree.')
}
Write-Ok ('entry symbol exported: ' + $entrySymbol)

# Debug information must match the requested setting.
$pdbPath = [System.IO.Path]::ChangeExtension($libPath, '.pdb')
if ($debugSymbols -eq 'yes') {
    if (-not (Test-Path -LiteralPath $pdbPath)) {
        Fail ('debug info is on but no debug symbols were produced: ' + $pdbPath)
    }
    $pdbItem = Get-Item -LiteralPath $pdbPath
    Write-Ok ('debug symbols present: ' + $pdbItem.Name + ' (' + [int]($pdbItem.Length / 1MB) + ' MB)')
} elseif (Test-Path -LiteralPath $pdbPath) {
    Write-Note ('debug info is off; removing the stale ' + (Split-Path $pdbPath -Leaf) + ' so bin/ reflects this build')
    Remove-Item -LiteralPath $pdbPath -Force
} else {
    Write-Ok 'debug symbols: none (as configured)'
}

# --------------------------------------------------------------------------
Write-Stage '3/3 extension_list.cfg (engine visibility)'
# --------------------------------------------------------------------------
$extensionList = Join-Path $projectDir '.godot\extension_list.cfg'
if (Test-Path -LiteralPath $extensionList) {
    Write-Ok ('already present: ' + $extensionList)
} elseif ($SkipImport) {
    Write-Note 'missing, but -SkipImport was passed; the engine will not see the extension until it exists.'
} else {
    Write-Info 'running the one-time headless editor import (AGENTS.md pitfall #1) ...'
    # The editor also writes its help-doc cache under %LOCALAPPDATA%\Godot, not
    # just %APPDATA%\Godot; both must point inside the repo or the sandbox denies
    # the writes ("Cannot create file" / "Cannot save editor help cache").
    $prevAppData = $env:APPDATA
    $prevLocalAppData = $env:LOCALAPPDATA
    $env:APPDATA = Join-Path $repoRoot '.godot-appdata'
    $env:LOCALAPPDATA = Join-Path (Join-Path $repoRoot '.godot-appdata') 'Local'
    try {
        $import = Invoke-Captured -FilePath $GodotExe -ArgumentList @('--headless', '--editor', '--quit-after', '300', '--path', $projectDir)
    } finally {
        $env:APPDATA = $prevAppData
        $env:LOCALAPPDATA = $prevLocalAppData
    }
    Set-Content -LiteralPath (Join-Path $logDir 'editor-import.log') -Value $import.Output -Encoding utf8
    if (-not (Test-Path -LiteralPath $extensionList)) {
        Fail ('the import did not create ' + $extensionList + ' - see ' + (Join-Path $logDir 'editor-import.log'))
    }
    Write-Ok 'extension_list.cfg created'
}

# --------------------------------------------------------------------------
if ($Bindings -and -not $SkipBindings) {
    Write-Stage 'bonus: .NET bindings'
    # Forward the same editor the build/import used, so glue generation and the
    # API install cannot disagree about which engine they target. Without an
    # explicit install target, leave the editor untouched (old behaviour).
    $psArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'build_dotnet_bindings.ps1'), '-GodotExe', $GodotExe)
    if ($InstallApiTo) { $psArgs += @('-InstallApiTo', $InstallApiTo) } else { $psArgs += '-SkipApiInstall' }
    $bind = Invoke-Captured -FilePath 'powershell.exe' -ArgumentList $psArgs
    $bindLog = Join-Path $logDir 'build-dotnet-bindings.log'
    Set-Content -LiteralPath $bindLog -Value $bind.Output -Encoding utf8
    if ($bind.ExitCode -ne 0) {
        Write-Host $bind.Output
        Fail ('build_dotnet_bindings.ps1 failed with exit code ' + $bind.ExitCode + ' - full log: ' + $bindLog)
    }
    Write-Ok '.NET bindings regenerated'
    if ($InstallApiTo) {
        Write-Ok ('API installed into ' + $InstallApiTo + ' - launch THAT editor for C# work')
    } else {
        Write-Note 'editor-mode C# needs the API next to the editor exe: pass -InstallApiTo <editor dir>.'
    }
}

Write-Host ''
Write-Host '== done ==' -ForegroundColor Green
Write-Host ('Usable extension: ' + $libPath)
Write-Host ('Entry symbol:     ' + $entrySymbol)
exit 0

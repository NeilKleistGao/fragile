<#
.SYNOPSIS
  One-click build for the fragile GDExtension: compile, verify, smoke-test.

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
                   [libraries] key in fragile.gdextension, and it exports the
                   entry_symbol that same file declares.
    3. import      One-time headless editor import so
                   project/.godot/extension_list.cfg exists. Without it the
                   engine never loads the extension and reports the misleading
                   'Cannot get class' (AGENTS.md pitfall #1).
    4. smoke test  Headless run of the project. Must exit 0 and must print
                   none of the known failure signatures.

  Exits 0 only when all of the above passed. Safe and cheap to re-run.

.PARAMETER Target
  template_debug (default; hot-reloadable) or template_release.

.PARAMETER Bindings
  Also regenerate the .NET bindings via scripts/build_dotnet_bindings.ps1.
  Do this whenever _bind_methods() signatures changed - the generated wrappers
  carry method-compatibility hashes and stale ones fail at runtime with a
  method-bind mismatch.

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

    [string] $GodotExe = 'C:\Program Files\Godot\Godot_v4.7-stable_mono_win64_console.exe',

    [switch] $SkipImport,
    [switch] $SkipSmokeTest,
    [switch] $AllowRunningEditor,

    [switch] $Bindings,
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
Write-Stage '0/4 preflight'
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

# --- clean stale shadow copies left by earlier failed replacements --------
$stale = @(Get-ChildItem -LiteralPath $binDir -Force -ErrorAction SilentlyContinue |
           Where-Object { $_.Name -like '~*' -or $_.Name -like '*.TMP' })
foreach ($old in $stale) {
    Write-Note ('removing stale shadow copy: ' + $old.Name + ' (' + $old.LastWriteTime + ')')
    Remove-Item -LiteralPath $old.FullName -Force
}

# --------------------------------------------------------------------------
Write-Stage '1/4 build'
# --------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$sconsLog = Join-Path $logDir 'scons-build.log'
$buildArgs = @()
if ($Target -ne 'template_debug') { $buildArgs += ('target=' + $Target) }
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
Write-Stage '2/4 verify the artefact'
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

$leftOver = @(Get-ChildItem -LiteralPath $binDir -Force -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -like '~*' -or $_.Name -like '*.TMP' })
if ($leftOver) {
    Fail ('shadow copies were left behind, so the link could not replace the DLL cleanly: ' + (($leftOver | ForEach-Object { $_.Name }) -join ', '))
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

# --------------------------------------------------------------------------
Write-Stage '3/4 extension_list.cfg (engine visibility)'
# --------------------------------------------------------------------------
$extensionList = Join-Path $projectDir '.godot\extension_list.cfg'
if (Test-Path -LiteralPath $extensionList) {
    Write-Ok ('already present: ' + $extensionList)
} elseif ($SkipImport) {
    Write-Note 'missing, but -SkipImport was passed; the smoke test will likely report "Cannot get class".'
} else {
    Write-Info 'running the one-time headless editor import (AGENTS.md pitfall #1) ...'
    $env:APPDATA = Join-Path $repoRoot '.godot-appdata'   # sandbox-safe user:// redirect
    $import = Invoke-Captured -FilePath $GodotExe -ArgumentList @('--headless', '--editor', '--quit-after', '300', '--path', $projectDir)
    Set-Content -LiteralPath (Join-Path $logDir 'editor-import.log') -Value $import.Output -Encoding utf8
    if (-not (Test-Path -LiteralPath $extensionList)) {
        Fail ('the import did not create ' + $extensionList + ' - see ' + (Join-Path $logDir 'editor-import.log'))
    }
    Write-Ok 'extension_list.cfg created'
}

# --------------------------------------------------------------------------
Write-Stage '4/4 smoke test'
# --------------------------------------------------------------------------
if ($SkipSmokeTest) {
    Write-Note 'skipped (-SkipSmokeTest).'
} else {
    $env:APPDATA = Join-Path $repoRoot '.godot-appdata'
    Write-Info 'running the project headless ...'
    $run = Invoke-Captured -FilePath $GodotExe -ArgumentList @('--headless', '--path', $projectDir, '--quit-after', '120')
    $smokeLog = Join-Path $logDir 'smoke-test.log'
    Set-Content -LiteralPath $smokeLog -Value $run.Output -Encoding utf8

    $signatures = @(
        @{ Name = 'extension not loaded (extension_list.cfg missing, wrong library name, or missing entry symbol)'; Pattern = "Cannot get class '" }
        @{ Name = 'library name/path in fragile.gdextension does not match the build output';                    Pattern = "Can't open dynamic library" }
        @{ Name = 'a placeholder node was created instead of the real class';                                    Pattern = 'cannot be created\. A placeholder will be created instead' }
        @{ Name = 'PackedScene failed to instantiate (dangling RefCounted member, or an empty scene)';           Pattern = 'Failed to instantiate scene state' }
        @{ Name = 'user://logs not writable - redirect APPDATA, this is a sandbox artefact';                     Pattern = "Could not create directory: 'user://logs'" }
        @{ Name = 'stale C# bindings - method-bind mismatch, regenerate with -Bindings';                         Pattern = 'MissingMethodException' }
        @{ Name = 'editor running stock GodotSharp/Api - use the -InstallApiTo editor for C# work';              Pattern = 'TypeLoadException' }
    )
    $found = @()
    foreach ($sig in $signatures) {
        if ($run.Output -match $sig.Pattern) { $found += $sig }
    }

    if ($run.ExitCode -ne 0) {
        Write-Host $run.Output
        Fail ('the headless run exited ' + $run.ExitCode + ' - full log: ' + $smokeLog)
    }
    if ($found.Count -gt 0) {
        Write-Host $run.Output
        foreach ($sig in $found) {
            Write-Host ('   [FAIL] signature: ' + $sig.Name) -ForegroundColor Red
        }
        Fail ('the headless run printed ' + $found.Count + ' known failure signature(s) - full log: ' + $smokeLog)
    }
    Write-Ok 'headless run exited 0 with no failure signatures'
}

# --------------------------------------------------------------------------
if ($Bindings) {
    Write-Stage 'bonus: .NET bindings'
    $psArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'build_dotnet_bindings.ps1'))
    if ($InstallApiTo) { $psArgs += @('-InstallApiTo', $InstallApiTo) }
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

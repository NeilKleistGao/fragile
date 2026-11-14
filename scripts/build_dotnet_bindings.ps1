<#
.SYNOPSIS
  fragile .NET bindings pipeline (limboai-style): generate C# bindings that
  include the fragile GDExtension classes (FInfo/FActor/FLevel/FWorld), build
  versioned GodotSharp NuGet packages and push them to a local NuGet feed.

.DESCRIPTION
  What it does, in order:
    1. Patches <GodotSource>/version.py so the packages get a custom SemVer
       prerelease (e.g. 4.7.0-fragile-v0.1.0.local).
    2. Runs the engine's hidden --generate-mono-glue tool WITH THE PROJECT
       LOADED, so the fragile classes are registered in ClassDB and their
       bindings are generated next to the engine-class bindings.
    3. Runs scripts/build_bindings_assemblies.py (thin wrapper around the
       engine's build_assemblies.py that skips the unrelated GodotTools step),
       which builds GodotSharp / GodotSharpEditor / Godot.SourceGenerators /
       Godot.NET.Sdk and copies the .nupkg files to the local feed.

  After it succeeds, the Godot project builds against the custom SDK via
  project/fragile.csproj (Sdk="Godot.NET.Sdk/<version>") and nuget.config
  at the repo root. C# code can then use the fragile types directly
  (GetNode<FWorld>(...), class MyLevel : FLevel, IntelliSense, ...).

  REGENERATE whenever _bind_methods() signatures change: the generated
  bindings carry the method-compatibility hashes of the current build.

.PARAMETER GodotSource
  Path to a Godot engine SOURCE checkout that matches the editor binary
  (4.7-stable -> the source tag/commit the binary was built from). Downloaded
  automatically via scripts/download_godot_source.py when missing (Python's
  OpenSSL stack can do TLS in the sandbox; curl/git/schannel cannot).

.PARAMETER GodotExe
  Godot 4.7 .NET editor binary used for glue generation. Must match
  GodotSource's engine version.

.PARAMETER BindingsVersion
  SemVer prerelease label appended to the engine version, e.g.
  "fragile-v0.1.0.local" -> package version 4.7.0-fragile-v0.1.0.local.
  Must NOT end in a digit: build_assemblies.py inserts a dot before trailing
  digits ("beta3" -> "beta.3"), which would corrupt e.g. "fragile-v0.1.0"
  into "fragile-v0.1..0".

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_dotnet_bindings.ps1

  The script is unsigned; bare './scripts/build_dotnet_bindings.ps1' fails
  under restrictive execution policies (SecurityError), so invoke it with
  -ExecutionPolicy Bypass as shown.
#>
[CmdletBinding()]
param(
    [string] $GodotSource = '',
    [string] $GodotExe = 'C:\Program Files\Godot\Godot_v4.7-stable_mono_win64_console.exe',
    [string] $BindingsVersion = 'fragile-v0.1.0.local',
    [string] $NuGetFeed = '',
    [string] $ApiOutputDir = '',
    [string] $InstallApiTo = '',
    [switch] $SkipGlue
)

$ErrorActionPreference = 'Stop'
# $PSScriptRoot is empty inside param() defaults on Windows PowerShell 5.1,
# so the repo-relative default paths are resolved here in the script body.
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$projectDir = Join-Path $repoRoot 'project'
if (-not $GodotSource) { $GodotSource = Join-Path $repoRoot '.deps\godot-4.7-stable' }
if (-not $NuGetFeed) { $NuGetFeed = Join-Path $repoRoot '.deps\nupkgs' }
if (-not $ApiOutputDir) { $ApiOutputDir = Join-Path $repoRoot '.deps\godot-api-output' }

Write-Host '== fragile .NET bindings pipeline =='
Write-Host ('repo:        ' + $repoRoot)
Write-Host ('godot src:   ' + $GodotSource)
Write-Host ('godot exe:   ' + $GodotExe)
Write-Host ('version tag: ' + $BindingsVersion)
Write-Host ('nuget feed:  ' + $NuGetFeed)

# --- 1. Preflight -----------------------------------------------------------
$versionPy = Join-Path $GodotSource 'version.py'
if (-not (Test-Path -LiteralPath $versionPy)) {
    Write-Host ('downloading Godot source to ' + $GodotSource + ' ...')
    $ErrorActionPreference = 'Continue'
    $out = & python (Join-Path $PSScriptRoot 'download_godot_source.py') --dest $GodotSource 2>&1 | Out-String
    $ErrorActionPreference = 'Stop'
    Write-Host $out
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $versionPy)) {
        throw 'Could not download the Godot source; run scripts/download_godot_source.py manually and re-run.'
    }
}
if (-not (Test-Path -LiteralPath $GodotExe)) {
    throw 'Godot editor binary not found: ' + $GodotExe
}
$extensionList = Join-Path $projectDir '.godot\extension_list.cfg'
if (-not (Test-Path -LiteralPath $extensionList)) {
    throw 'Missing "' + $extensionList + '". Run the one-time headless editor import first (AGENTS.md pitfall #1), otherwise the glue generation will not see the fragile classes.'
}
$libDir = Join-Path $projectDir 'addons\fragile\bin'
$libs = Get-ChildItem -LiteralPath $libDir -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in '.dll', '.so', '.dylib' }
if (-not $libs) {
    throw 'No built fragile library in project/addons/fragile/bin - run scons first.'
}

$vpy = Get-Content -LiteralPath $versionPy -Raw
$major = [regex]::Match($vpy, '(?m)^major = (\d+)').Groups[1].Value
$minor = [regex]::Match($vpy, '(?m)^minor = (\d+)').Groups[1].Value
$patch = [regex]::Match($vpy, '(?m)^patch = (\d+)').Groups[1].Value
$engineVersion = $major + '.' + $minor + '.' + $patch
Write-Host ('engine source version: ' + $engineVersion)

# Mirror build_assemblies.py's status transform: a dot is inserted before any
# trailing digits ("beta3" -> "beta.3"). If that fires on our label the final
# package version silently differs from $pkgVersion below - refuse instead.
$label = $BindingsVersion
if ($label -match '\d+$') {
    $pos = $label.Length - $Matches[0].Length
    $label = $label.Insert($pos, '.')
}
if ($label -ne $BindingsVersion) {
    throw 'BindingsVersion "' + $BindingsVersion + '" ends in digits; build_assemblies.py would turn it into "' + $label + '". Use a label without trailing digits (e.g. fragile-v0.1.0.local).'
}
$pkgVersion = $engineVersion + '-' + $BindingsVersion
Write-Host ('package version will be: ' + $pkgVersion)

# --- 2. Pin the package version in the engine source ------------------------
$statusLine = 'status = "' + $BindingsVersion + '"'
if ($vpy -notmatch '(?m)^status = ".*"$') {
    throw 'version.py has no status line to patch.'
}
$vpyPatched = [regex]::Replace($vpy, '(?m)^status = ".*"$', $statusLine)
Set-Content -LiteralPath $versionPy -Value $vpyPatched -Encoding utf8 -NoNewline
Write-Host ('[1/3] version.py status -> ' + $statusLine)

# --- 3. Generate the C# glue (hidden option; requires --editor) -------------
if (-not $SkipGlue) {
    $glueDir = Join-Path $GodotSource 'modules\mono\glue'
    Write-Host ('[2/3] generating mono glue into ' + $glueDir + ' (this takes a few minutes) ...')
    $env:APPDATA = Join-Path $repoRoot '.godot-appdata'   # sandbox-safe user:// redirect
    $ErrorActionPreference = 'Continue'                  # native stderr must not stop us
    $out = & $GodotExe --headless --editor --path $projectDir --generate-mono-glue $glueDir 2>&1 | Out-String
    $ErrorActionPreference = 'Stop'
    $glueLog = Join-Path $repoRoot '.deps\generate-mono-glue.log'
    New-Item -ItemType Directory -Force -Path (Split-Path $glueLog) | Out-Null
    Set-Content -LiteralPath $glueLog -Value $out -Encoding utf8
    Write-Host ('    engine exit code: ' + $LASTEXITCODE + '; log: ' + $glueLog)
    $probe = Join-Path $glueDir 'GodotSharp\GodotSharp\Generated\GodotObjects\FWorld.cs'
    if (-not (Test-Path -LiteralPath $probe)) {
        throw 'Glue generation did not produce "' + $probe + '" - the fragile classes were not registered (check extension_list.cfg and the log).'
    }
    Write-Host '    OK: FWorld.cs (and FInfo/FActor/FLevel) generated.'
} else {
    Write-Host '[2/3] skipped glue generation (-SkipGlue).'
}

# --- Patch the glue for extension classes (idempotent) ----------------------
# See scripts/patch_bindings_glue.py: the official generator skips extension
# classes in Constructors.cs and the engine stores an ancestor creation_func
# for them; both would break C# wrapping/construction at runtime.
Write-Host '[glue] patching extension-class gaps (patch_bindings_glue.py) ...'
$ErrorActionPreference = 'Continue'
$out = & python (Join-Path $PSScriptRoot 'patch_bindings_glue.py') $GodotSource 2>&1 | Out-String
$ErrorActionPreference = 'Stop'
Write-Host $out
if ($LASTEXITCODE -ne 0) {
    throw 'patch_bindings_glue.py failed (exit ' + $LASTEXITCODE + ')'
}

# --- 4. Build and push the NuGet packages -----------------------------------
Write-Host '[3/3] building GodotSharp packages (build_bindings_assemblies.py) ...'
New-Item -ItemType Directory -Force -Path $NuGetFeed | Out-Null
New-Item -ItemType Directory -Force -Path $ApiOutputDir | Out-Null
$buildScript = Join-Path $PSScriptRoot 'build_bindings_assemblies.py'
$ErrorActionPreference = 'Continue'
$out = & python $buildScript --godot-source $GodotSource --godot-output-dir $ApiOutputDir --push-nupkgs-local $NuGetFeed 2>&1 | Out-String
$ErrorActionPreference = 'Stop'
$buildLog = Join-Path $repoRoot '.deps\build-assemblies.log'
Set-Content -LiteralPath $buildLog -Value $out -Encoding utf8
Write-Host ('    exit code: ' + $LASTEXITCODE + '; log: ' + $buildLog)
if ($LASTEXITCODE -ne 0) {
    throw 'build_assemblies.py failed (exit ' + $LASTEXITCODE + ') - see ' + $buildLog
}

# NOTE: elements are newline-separated expressions in @(); a comma after
# 'a' + $v + 'b' would bind tighter than + and merge the list into one string.
$expected = @(
    ('GodotSharp.' + $pkgVersion + '.nupkg')
    ('GodotSharpEditor.' + $pkgVersion + '.nupkg')
    ('Godot.SourceGenerators.' + $pkgVersion + '.nupkg')
    ('Godot.NET.Sdk.' + $pkgVersion + '.nupkg')
)
$missing = $expected | Where-Object { -not (Test-Path -LiteralPath (Join-Path $NuGetFeed $_)) }
if ($missing) {
    throw 'Packages missing from feed: ' + ($missing -join ', ')
}

# --- 5. Optionally install the API assemblies into an editor directory -----
# The editor ALWAYS loads its runtime bindings from <exe dir>/GodotSharp/Api
# (modules/mono/godotsharp_dirs.cpp, TOOLS_ENABLED branch) - there is no
# override. Editor-mode runs only see the fragile types if that folder holds
# our assemblies, so point -InstallApiTo at a portable editor directory (or,
# with elevation, the installed one). Exported games are unaffected: they use
# the GodotSharp.dll published from the NuGet package.
if ($InstallApiTo) {
    foreach ($cfg in 'Debug', 'Release') {
        $src = Join-Path (Join-Path $ApiOutputDir 'GodotSharp\Api') $cfg
        $dst = Join-Path (Join-Path $InstallApiTo 'GodotSharp\Api') $cfg
        if (-not (Test-Path -LiteralPath $src)) {
            throw 'missing built Api output: ' + $src
        }
        New-Item -ItemType Directory -Force -Path $dst | Out-Null
        Copy-Item -Path (Join-Path $src '*') -Destination $dst -Force
        Write-Host ('[4/4] installed Api/' + $cfg + ' -> ' + $dst)
    }
}

Write-Host ''
Write-Host '== done =='
Write-Host 'NuGet feed now contains:'
Get-ChildItem -LiteralPath $NuGetFeed -Filter '*.nupkg' | ForEach-Object { Write-Host ('  ' + $_.Name) }
Write-Host ''
Write-Host ('Next: dotnet build "' + (Join-Path $projectDir 'fragile.csproj') + '"')
Write-Host ('      (project/fragile.csproj must reference Godot.NET.Sdk/' + $pkgVersion + ')')

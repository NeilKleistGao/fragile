@echo off
rem ===========================================================================
rem  One-click build for the fragile GDExtension.
rem
rem  Double-click this file (or run it from a terminal). It:
rem    1. compiles the extension;
rem    2. verifies the artefact against the SConstruct /
rem       src/register_types.cpp / fragile.gdextension contract;
rem    3. regenerates the .NET bindings and installs them into
rem       .deps\godot-4.7-fragile - the editor to launch for C# work
rem       (editor-mode runs read bindings from <exe dir>/GodotSharp/Api).
rem  It exits 0 only when the extension is verifiably usable.
rem
rem  Step 3 runs every time ON PURPOSE. Skipping it after editing
rem  _bind_methods() is what leaves the Godot editor showing methods you
rem  already deleted - the stale GodotSharp/Api keeps them alive.
rem
rem  Extra arguments are forwarded to scripts\build.ps1, e.g.:
rem    build.cmd -Target template_release
rem    build.cmd -DebugInfo off
rem    build.cmd -SkipBindings
rem
rem  -SkipBindings is the opt-out when you only want the native library
rem  rebuilt (much faster). To install the API into a different editor
rem  directory, call scripts\build.ps1 directly - -Bindings and
rem  -InstallApiTo are already passed below and PowerShell rejects a
rem  parameter given twice.
rem
rem  Automation should call scripts\build.ps1 directly (it does not pause).
rem ===========================================================================
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" -Bindings -InstallApiTo "%~dp0.deps\godot-4.7-fragile" %*
set "ERR=%ERRORLEVEL%"
echo.
if not "%ERR%"=="0" (
    echo Build FAILED with exit code %ERR%.
) else (
    echo Build succeeded.
)
pause
exit /b %ERR%

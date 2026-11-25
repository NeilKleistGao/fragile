@echo off
rem ===========================================================================
rem  One-click build for the fragile GDExtension.
rem
rem  Double-click this file (or run it from a terminal). It compiles the
rem  extension, verifies the artefact against the SConstruct /
rem  src/register_types.cpp / fragile.gdextension contract, and smoke-tests it
rem  headlessly. It exits 0 only when the extension is verifiably usable.
rem
rem  Extra arguments are forwarded to scripts\build.ps1, e.g.:
rem    build.cmd -Target template_release
rem    build.cmd -Bindings -InstallApiTo .deps\godot-4.7-fragile
rem
rem  Automation should call scripts\build.ps1 directly (it does not pause).
rem ===========================================================================
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" %*
set "ERR=%ERRORLEVEL%"
echo.
if not "%ERR%"=="0" (
    echo Build FAILED with exit code %ERR%.
) else (
    echo Build succeeded.
)
pause
exit /b %ERR%


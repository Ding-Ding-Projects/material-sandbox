@echo off
setlocal EnableExtensions

rem This obsolete installer entry point is intentionally disabled.
rem Use the repository-root build-installer.bat for permanently unsigned packaging.
echo [legacy-installer] This obsolete installer entry point is disabled.
echo [legacy-installer] Use "%~dp0..\..\build-installer.bat" for the supported permanently unsigned installer.
exit /b 64

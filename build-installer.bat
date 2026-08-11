@echo off
setlocal EnableExtensions
set "SCRIPT_ROOT=%~dp0"

rem Build one verified, permanently unsigned installer. /s, --silent, or SILENT=1 suppresses prompts.
set "SILENT_ARG="
set "PLAN_ARG="
rem Installer packaging is intentionally fixed to x64; ambient SBIE_ARCH is ignored.
set "ARCH=x64"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="/s" goto arg_silent
if /I "%~1"=="--silent" goto arg_silent
if /I "%~1"=="--plan" goto arg_plan
echo [installer] Unknown argument "%~1". Supported: /s, --silent, --plan.
exit /b 64

:arg_silent
set "SILENT_ARG=-Silent"
shift
goto parse_args

:arg_plan
set "PLAN_ARG=-PlanOnly"
shift
goto parse_args

:args_done
if /I "%SILENT%"=="1" set "SILENT_ARG=-Silent"
if /I "%SBIE_BOOTSTRAP_PLAN%"=="1" set "PLAN_ARG=-PlanOnly"
set "POWERSHELL_EXE=powershell.exe"
where pwsh.exe >nul 2>&1 && set "POWERSHELL_EXE=pwsh.exe"

"%POWERSHELL_EXE%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_ROOT%scripts\windows-build-bootstrap.ps1" -Mode Installer -Architecture "%ARCH%" %SILENT_ARG% %PLAN_ARG%
if errorlevel 1 (
  echo [installer] Unsigned installer build failed with errorlevel %errorlevel%.
  exit /b %errorlevel%
)

exit /b 0

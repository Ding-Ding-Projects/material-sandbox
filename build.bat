@echo off
setlocal EnableExtensions

rem Fresh-Windows build entry point. /s, --silent, or SILENT=1 suppresses prompts.
set "SILENT_ARG="
set "PLAN_ARG="
set "ARCH=%SBIE_ARCH%"
if not defined ARCH set "ARCH=x64"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="/s" goto arg_silent
if /I "%~1"=="--silent" goto arg_silent
if /I "%~1"=="--plan" goto arg_plan
echo [build] Unknown argument "%~1". Supported: /s, --silent, --plan.
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
if /I not "%ARCH%"=="x64" if /I not "%ARCH%"=="ARM64" (
  echo [build] Unsupported SBIE_ARCH "%ARCH%". Use x64 or ARM64.
  exit /b 2
)

set "POWERSHELL_EXE=powershell.exe"
where pwsh.exe >nul 2>&1 && set "POWERSHELL_EXE=pwsh.exe"

"%POWERSHELL_EXE%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows-build-bootstrap.ps1" -Mode Build -Architecture "%ARCH%" %SILENT_ARG% %PLAN_ARG%
if errorlevel 1 (
  echo [build] Windows build failed with errorlevel %errorlevel%.
  exit /b %errorlevel%
)

exit /b 0

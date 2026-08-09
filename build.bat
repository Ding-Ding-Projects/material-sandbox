@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Sandboxie Windows build entry point.  /s, --silent, or SILENT=1 suppresses prompts.
set "SILENT_MODE=0"
if /I "%SILENT%"=="1" set "SILENT_MODE=1"
if /I "%1"=="/s" set "SILENT_MODE=1"
if /I "%1"=="--silent" set "SILENT_MODE=1"
set "ARCH=%SBIE_ARCH%"
if not defined ARCH set "ARCH=x64"
if /I not "%ARCH%"=="x64" if /I not "%ARCH%"=="ARM64" (
  echo [build] Unsupported SBIE_ARCH "%ARCH%". Use x64 or ARM64.
  exit /b 2
)

set "ROOT=%~dp0"
set "QMAKE=%ROOT%Qt\%qt_version%\msvc2022_64\bin\qmake.exe"
if not defined qt_version set "QMAKE=%ROOT%Qt\6.8.3\msvc2022_64\bin\qmake.exe"
if not exist "%QMAKE%" (
  echo [build] Missing Qt qmake at "%QMAKE%".
  echo [build] Install the repository's pinned Qt 6.8.3 MSVC kit or run on the hosted Windows build image.
  exit /b 3
)
if not exist "%ROOT%SandboxiePlus\install_jom.cmd" (
  echo [build] Missing SandboxiePlus\install_jom.cmd.
  exit /b 4
)

echo [build] Preparing pinned jom and MSVC environment for %ARCH%...
call "%ROOT%SandboxiePlus\install_jom.cmd"
if errorlevel 1 exit /b 5
if not exist "%ROOT%Qt\Tools\QtCreator\bin\jom.exe" (
  echo [build] jom bootstrap did not produce Qt\Tools\QtCreator\bin\jom.exe.
  exit /b 6
)

echo [build] Building SandboxiePlus through qmake_plus.cmd...
call "%ROOT%SandboxiePlus\qmake_plus.cmd" %ARCH% build_qt6
if errorlevel 1 (
  echo [build] qmake_plus.cmd failed with errorlevel %errorlevel%.
  exit /b %errorlevel%
)
if not exist "%ROOT%SandboxiePlus\Bin\x64\Release\SandMan.exe" if /I "%ARCH%"=="x64" (
  echo [build] SandMan.exe was not produced in SandboxiePlus\Bin\x64\Release.
  exit /b 7
)
if not exist "%ROOT%SandboxiePlus\Bin\ARM64\Release\SandMan.exe" if /I "%ARCH%"=="ARM64" (
  echo [build] SandMan.exe was not produced in SandboxiePlus\Bin\ARM64\Release.
  exit /b 8
)
echo [build] Build complete for %ARCH%.
if "%SILENT_MODE%"=="1" exit /b 0
choice /M "Launch the built SandMan executable now"
if errorlevel 2 exit /b 0
if /I "%ARCH%"=="x64" start "Sandboxie-Plus" "%ROOT%SandboxiePlus\Bin\x64\Release\SandMan.exe"
if /I "%ARCH%"=="ARM64" start "Sandboxie-Plus" "%ROOT%SandboxiePlus\Bin\ARM64\Release\SandMan.exe"
exit /b 0

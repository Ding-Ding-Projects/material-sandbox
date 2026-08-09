@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the unsigned Windows installer from the same x64 path used by CI.
set "SILENT_MODE=0"
if /I "%SILENT%"=="1" set "SILENT_MODE=1"
if /I "%1"=="/s" set "SILENT_MODE=1"
if /I "%1"=="--silent" set "SILENT_MODE=1"
set "ROOT=%~dp0"
set "ARCH=x64"

call "%ROOT%build.bat" /s
if errorlevel 1 exit /b %errorlevel%
call "%ROOT%Installer\copy_build.cmd" x64 build_qt6
if errorlevel 1 (
  echo [installer] Installer staging failed with errorlevel %errorlevel%.
  exit /b %errorlevel%
)
set "ISCC="
for %%P in ("%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" "%ProgramFiles%\Inno Setup 6\ISCC.exe") do if not defined ISCC if exist "%%~P" set "ISCC=%%~P"
if not defined ISCC (
  echo [installer] Missing Inno Setup 6 ISCC.exe.
  echo [installer] Install Inno Setup 6 from its canonical upstream, then rerun this script.
  exit /b 20
)
set "VERSION=0.0.0-local"
for /F "tokens=2 delims==" %%V in ('findstr /B /C:"#define MyAppVersion" "%ROOT%Installer\Sandboxie-Plus.iss"') do set "VERSION=%%~V"
set "TEMP_ISS=%TEMP%\Sandboxie-Plus-unsigned-%RANDOM%.iss"
findstr /V /C:"SignTool=" "%ROOT%Installer\Sandboxie-Plus.iss" > "%TEMP_ISS%"
echo [installer] Building unsigned installer; no signing tool is invoked.
"%ISCC%" /O"%ROOT%Installer\Output" "%TEMP_ISS%" /DMyAppVersion=%VERSION% /DMyAppArch=x64 /DMyAppSrc=SbiePlus_x64
set "RESULT=%ERRORLEVEL%"
del /Q "%TEMP_ISS%" >nul 2>&1
if not "%RESULT%"=="0" (
  echo [installer] Inno Setup failed with errorlevel %RESULT%.
  exit /b %RESULT%
)
for %%F in ("%ROOT%Installer\Output\Sandboxie-Plus-x64-v*.exe") do (
  echo [installer] Unsigned installer: %%~fF
  certutil -hashfile "%%~fF" SHA256 | findstr /R /V /C:"CertUtil:" 
)
if "%SILENT_MODE%"=="1" exit /b 0
echo [installer] Build complete. The installer is unsigned by permanent project policy.
exit /b 0

@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the permanently unsigned Windows installer from the same x64 path used by CI.
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
echo [installer] Building the permanently unsigned installer; signing hooks are disabled in the canonical source.
"%ISCC%" /O"%ROOT%Installer\Output" "%ROOT%Installer\Sandboxie-Plus.iss" /DMyAppVersion=%VERSION% /DMyAppArch=x64 /DMyAppSrc=SbiePlus_x64
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
  echo [installer] Inno Setup failed with errorlevel %RESULT%.
  exit /b %RESULT%
)
for %%F in ("%ROOT%Installer\Output\Sandboxie-Plus-x64-v*.exe") do (
  set "SBIE_UNSIGNED_INSTALLER=%%~fF"
  "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -NonInteractive -Command "$result = Get-AuthenticodeSignature -LiteralPath $env:SBIE_UNSIGNED_INSTALLER; if ($result.Status -ne 'NotSigned') { Write-Error 'Installer violates the permanent unsigned-packaging policy.'; exit 1 }"
  if errorlevel 1 exit /b 30
  echo [installer] Verified unsigned installer: %%~fF
  "%SystemRoot%\System32\certutil.exe" -hashfile "%%~fF" SHA256 | findstr /R /V /C:"CertUtil:"
)
set "SBIE_UNSIGNED_INSTALLER="
if "%SILENT_MODE%"=="1" exit /b 0
echo [installer] Build complete. The installer is unsigned by permanent project policy.
exit /b 0

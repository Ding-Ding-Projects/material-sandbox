@echo off
setlocal EnableExtensions EnableDelayedExpansion

call "%~dp0buildVariables.cmd" %*
if errorlevel 1 exit /b %errorlevel%
@echo off

rem install-qt-action may place the target package beside the checkout, while
rem older x64 runs place it under the checkout. Prefer its explicit target root
rem and derive the common Qt installation root before falling back to the local
rem layout. This keeps ARM64 staging from quietly looking in the wrong tree.
if not defined SBIE_QT_ROOT if defined QT_ROOT_DIR for %%Q in ("%QT_ROOT_DIR%\..\..") do set "SBIE_QT_ROOT=%%~fQ"
if not defined SBIE_QT_ROOT set "SBIE_QT_ROOT=%~dp0..\Qt"
if not defined SBIE_7ZIP_EXE set "SBIE_7ZIP_EXE=C:\Program Files\7-Zip\7z.exe"

if /I "%openssl_version:~0,3%"=="1.1" (set "sslMajorVersion=1_1") else (set "sslMajorVersion=3")

if /I "%~1"=="x86" (
  set "archPath=Win32"
  set "qtPath=%SBIE_QT_ROOT%\%qt_version%\msvc2022"
  set "instPath=%~dp0SbiePlus_x86"
  if not defined SBIE_TOOLCHAIN_READY call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
)
if /I "%~1"=="x64" (
  set "archPath=x64"
  set "qtPath=%SBIE_QT_ROOT%\%qt_version%\msvc2022_64"
  set "instPath=%~dp0SbiePlus_x64"
  if not defined SBIE_TOOLCHAIN_READY call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)
if /I "%~1"=="ARM64" (
  set "archPath=ARM64"
  set "qtPath=%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64"
  set "instPath=%~dp0SbiePlus_a64"
  set "sslMajorVersion=1_1"
  if not defined SBIE_TOOLCHAIN_READY call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsamd64_arm64.bat"
)

if not defined archPath (
  echo [stage] Unsupported architecture "%~1".
  exit /b 2
)
if defined SBIE_INSTALL_STAGE set "instPath=%SBIE_INSTALL_STAGE%"
if not exist "%SBIE_7ZIP_EXE%" (
  echo [stage] Missing verified 7-Zip executable at "%SBIE_7ZIP_EXE%".
  exit /b 3
)
if not defined VCToolsRedistDir (
  call :discover_redist || exit /b 4
)

set "redistPath=%VCToolsRedistDir%\%~1\Microsoft.VC143.CRT"
set "srcPath=%~dp0..\SandboxiePlus\Bin\%archPath%\Release"
set "sbiePath=%~dp0..\Sandboxie\Bin\%archPath%\SbieRelease"

if exist "%instPath%\." (
  for /F "delims=" %%F in ('dir /B /A "%instPath%" 2^>nul') do (
    echo [stage] Refusing non-empty stage "%instPath%"; use a fresh run-scoped path.
    exit /b 5
  )
) else (
  mkdir "%instPath%" || exit /b 5
)

echo [stage] Architecture: %archPath%
echo [stage] Destination: %instPath%
echo [stage] Qt: %qtPath%
echo [stage] VC runtime: %redistPath%

call :copy_pattern "%redistPath%\*" "%instPath%" || exit /b 10

for %%F in (Qt6Core.dll Qt6Gui.dll Qt6Network.dll Qt6Widgets.dll Qt6Qml.dll Qt6Concurrent.dll) do (
  call :copy_file "%qtPath%\bin\%%F" "%instPath%" || exit /b 11
)

call :ensure_dir "%instPath%\platforms" || exit /b 12
for %%F in (qdirect2d.dll qminimal.dll qoffscreen.dll qwindows.dll) do (
  call :copy_file "%qtPath%\plugins\platforms\%%F" "%instPath%\platforms" || exit /b 12
)
call :ensure_dir "%instPath%\styles" || exit /b 13
call :copy_file "%qtPath%\plugins\styles\qmodernwindowsstyle.dll" "%instPath%\styles" || exit /b 13
call :ensure_dir "%instPath%\tls" || exit /b 14
for %%F in (qcertonlybackend.dll qopensslbackend.dll qschannelbackend.dll) do (
  call :copy_file "%qtPath%\plugins\tls\%%F" "%instPath%\tls" || exit /b 14
)

if /I "%archPath%"=="Win32" (
  call :copy_file "%~dp0OpenSSL\Win_x86\bin\libssl-%sslMajorVersion%.dll" "%instPath%" || exit /b 15
  call :copy_file "%~dp0OpenSSL\Win_x86\bin\libcrypto-%sslMajorVersion%.dll" "%instPath%" || exit /b 15
) else (
  call :copy_file "%~dp0OpenSSL\Win_%archPath%\bin\libssl-%sslMajorVersion%-%archPath%.dll" "%instPath%" || exit /b 15
  call :copy_file "%~dp0OpenSSL\Win_%archPath%\bin\libcrypto-%sslMajorVersion%-%archPath%.dll" "%instPath%" || exit /b 15
)
call :copy_file "%~dp07-Zip\7-Zip-%archPath%\7z.dll" "%instPath%" || exit /b 16

for %%F in (MiscHelpers.dll MiscHelpers.pdb QSbieAPI.dll QSbieAPI.pdb QtSingleApp.dll QtSingleApp.pdb UGlobalHotkey.dll UGlobalHotkey.pdb SandMan.exe SandMan.pdb) do (
  call :copy_file "%srcPath%\%%F" "%instPath%" || exit /b 17
)

call :ensure_dir "%instPath%\translations" || exit /b 18
call :copy_pattern "%~dp0..\SandboxiePlus\Build_SandMan_%archPath%\release\sandman_*.qm" "%instPath%\translations" || exit /b 18
call :copy_pattern "%~dp0qttranslations\qm\qt_*.qm" "%instPath%\translations" || exit /b 18
call :copy_pattern "%~dp0qttranslations\qm\qtbase_*.qm" "%instPath%\translations" || exit /b 18
call :copy_pattern "%~dp0qttranslations\qm\qtmultimedia_*.qm" "%instPath%\translations" || exit /b 18
if /I not "%archPath%"=="ARM64" (
  call :copy_optional_pattern "%qtPath%\translations\qtscript_*.qm" "%instPath%\translations" || exit /b 18
  call :copy_optional_pattern "%qtPath%\translations\qtxmlpatterns_*.qm" "%instPath%\translations" || exit /b 18
)

"%SBIE_7ZIP_EXE%" a -y "%instPath%\translations.7z" "%instPath%\translations\*"
if errorlevel 1 exit /b 19
if not exist "%instPath%\translations.7z" exit /b 19
rmdir /S /Q "%instPath%\translations"
if errorlevel 1 exit /b 19

"%SBIE_7ZIP_EXE%" a -y "%instPath%\troubleshooting.7z" "%~dp0..\SandboxiePlus\SandMan\Troubleshooting\*"
if errorlevel 1 exit /b 20
if not exist "%instPath%\troubleshooting.7z" exit /b 20

for %%F in (
  SbieSvc.exe SbieSvc.pdb SbieDll.dll SbieDll.pdb SbieDrv.sys SbieDrv.pdb
  SbieCtrl.exe SbieCtrl.pdb Start.exe Start.pdb kmdutil.exe kmdutil.pdb
  SbieIni.exe SbieIni.pdb SbieMsg.dll SboxHostDll.dll SboxHostDll.pdb
  SandboxieBITS.exe SandboxieBITS.pdb SandboxieCrypto.exe SandboxieCrypto.pdb
  SandboxieDcomLaunch.exe SandboxieDcomLaunch.pdb SandboxieRpcSs.exe SandboxieRpcSs.pdb
  SandboxieWUAU.exe SandboxieWUAU.pdb
) do (
  call :copy_file "%sbiePath%\%%F" "%instPath%" || exit /b 21
)

if /I "%archPath%"=="x64" (
  call :ensure_dir "%instPath%\32" || exit /b 22
  for %%F in (SbieSvc.exe SbieSvc.pdb SbieDll.dll SbieDll.pdb) do (
    call :copy_file "%~dp0..\Sandboxie\Bin\Win32\SbieRelease\%%F" "%instPath%\32" || exit /b 22
  )
  call :copy_file "%~dp0..\SandboxiePlus\x64\Release\SbieShellExt.dll" "%instPath%" || exit /b 22
  call :copy_file "%~dp0..\SandboxiePlus\x64\Release\SbieShellPkg.msix" "%instPath%" || exit /b 22
)
if /I "%archPath%"=="ARM64" (
  call :ensure_dir "%instPath%\32" || exit /b 23
  for %%F in (SbieSvc.exe SbieSvc.pdb SbieDll.dll SbieDll.pdb) do (
    call :copy_file "%~dp0..\Sandboxie\Bin\Win32\SbieRelease\%%F" "%instPath%\32" || exit /b 23
  )
  call :ensure_dir "%instPath%\64" || exit /b 23
  for %%F in (SbieDll.dll SbieDll.pdb) do (
    call :copy_file "%~dp0..\Sandboxie\Bin\ARM64EC\SbieRelease\%%F" "%instPath%\64" || exit /b 23
  )
  call :copy_file "%~dp0..\SandboxiePlus\ARM64\Release\SbieShellExt.dll" "%instPath%" || exit /b 23
  call :copy_file "%~dp0..\SandboxiePlus\ARM64\Release\SbieShellPkg.msix" "%instPath%" || exit /b 23
)

for %%F in (Templates.ini Manifest0.txt Manifest1.txt Manifest2.txt SbieSettings.ini) do (
  call :copy_file "%~dp0..\Sandboxie\install\%%F" "%instPath%" || exit /b 24
)

for %%F in (ImBox.exe ImBox.pdb UpdUtil.exe UpdUtil.pdb MiniDump.exe MiniDump.pdb) do (
  call :copy_file "%~dp0..\SandboxieTools\%archPath%\Release\%%F" "%instPath%" || exit /b 25
)

echo [stage] Complete: %instPath%
exit /b 0

:discover_redist
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [stage] Missing Visual Studio discovery tool "%VSWHERE%" and VCToolsRedistDir is unset.
  exit /b 1
)
set "VS_INSTALLATION="
for /F "usebackq delims=" %%V in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Redist.14.Latest -property installationPath`) do if not defined VS_INSTALLATION set "VS_INSTALLATION=%%V"
if not defined VS_INSTALLATION (
  echo [stage] No compatible Visual Studio 2022 VC143 redistributable was discovered.
  exit /b 1
)
set "REDIST_BASE=%VS_INSTALLATION%\VC\Redist\MSVC"
for /F "delims=" %%D in ('dir /B /AD /O-N "%REDIST_BASE%" 2^>nul') do if not defined VCToolsRedistDir set "VCToolsRedistDir=%REDIST_BASE%\%%D"
if not defined VCToolsRedistDir (
  echo [stage] No VC143 redistributable directory was found below "%REDIST_BASE%".
  exit /b 1
)
exit /b 0

:ensure_dir
if not exist "%~1\." mkdir "%~1"
if errorlevel 1 exit /b 1
exit /b 0

:copy_file
if not exist "%~1" (
  echo [stage] Missing required file "%~1".
  exit /b 1
)
copy /Y "%~1" "%~2\" >nul
if errorlevel 1 (
  echo [stage] Copy failed: "%~1" to "%~2".
  exit /b 1
)
if not exist "%~2\%~nx1" (
  echo [stage] Copy did not produce "%~2\%~nx1".
  exit /b 1
)
exit /b 0

:copy_pattern
set "COPY_COUNT=0"
for %%F in ("%~1") do if exist "%%~fF" (
  call :copy_file "%%~fF" "%~2" || exit /b 1
  set /A COPY_COUNT+=1
)
if !COPY_COUNT! EQU 0 (
  echo [stage] Pattern matched no required files: "%~1".
  exit /b 1
)
exit /b 0

:copy_optional_pattern
for %%F in ("%~1") do if exist "%%~fF" (
  call :copy_file "%%~fF" "%~2" || exit /b 1
)
exit /b 0

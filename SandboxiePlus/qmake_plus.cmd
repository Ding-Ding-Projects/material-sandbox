@echo off
setlocal EnableExtensions

call "%~dp0..\Installer\buildVariables.cmd" %*
if errorlevel 1 exit /b %errorlevel%
@echo off

if not defined SBIE_QT_ROOT set "SBIE_QT_ROOT=%~dp0..\Qt"
set "jom=%SBIE_QT_ROOT%\Tools\QtCreator\bin\jom.exe"

if /I "%~1"=="Win32" (
  set "qt_path=%SBIE_QT_ROOT%\%qt_version%\msvc2022"
  set "private_headers=%SBIE_QT_ROOT%\%qt_version%\msvc2022\include\QtCore\%qt_version%\QtCore"
  set "private_target=%SBIE_QT_ROOT%\%qt_version%\msvc2022\include\QtCore"
  set "build_arch=Win32"
  set "qt_params="
  set "vcvars_arg=x86"
)
if /I "%~1"=="x64" (
  set "qt_path=%SBIE_QT_ROOT%\%qt_version%\msvc2022_64"
  set "private_headers=%SBIE_QT_ROOT%\%qt_version%\msvc2022_64\include\QtCore\%qt_version%\QtCore"
  set "private_target=%SBIE_QT_ROOT%\%qt_version%\msvc2022_64\include\QtCore"
  set "build_arch=x64"
  set "qt_params="
  set "vcvars_arg=amd64"
)
if /I "%~1"=="ARM64" (
  set "qt_path=%SBIE_QT_ROOT%\%qt6_version%\msvc2022_64"
  set "private_headers=%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\include\QtCore\%qt6_version%\QtCore"
  set "private_target=%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\include\QtCore"
  set "build_arch=ARM64"
  set "target_qt_conf=%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf"
  >"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo [DevicePaths]
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo Prefix=%SBIE_QT_ROOT:\=/%/%qt6_version%
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo [Paths]
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo Prefix=../
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo HostPrefix=../../msvc2022_64
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo HostData=../msvc2022_arm64
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo Sysroot=
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo SysrootifyPrefix=false
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo TargetSpec=win32-arm64-msvc
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo HostSpec=win32-msvc
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo Documentation=../../Docs/Qt-%qt6_version%
  >>"%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf" echo Examples=../../Examples/Qt-%qt6_version%
  set "qt_params=-qtconf "%SBIE_QT_ROOT%\%qt6_version%\msvc2022_arm64\bin\my_target_qt.conf""
  set "vcvars_arg=amd64_arm64"
)

if not defined build_arch (
  echo [qt] Unsupported architecture "%~1".
  exit /b 2
)
if not defined SBIE_TOOLCHAIN_READY (
  call :load_vs_environment || exit /b 6
)
if not exist "%qt_path%\bin\qmake.exe" (
  echo [qt] Missing qmake at "%qt_path%\bin\qmake.exe".
  exit /b 3
)
if not exist "%jom%" (
  echo [qt] Missing jom at "%jom%".
  exit /b 4
)
if not exist "%private_headers%\." (
  echo [qt] Missing private Qt headers at "%private_headers%".
  exit /b 5
)
xcopy /E /I /Y /Q "%private_headers%" "%private_target%" >nul
if errorlevel 1 exit /b 5

call :build_qmake "Build_UGlobalHotkey_%build_arch%" "%~dp0UGlobalHotkey\uglobalhotkey.qc.pro" "%~dp0bin\%build_arch%\Release\UGlobalHotkey.dll" || exit /b 10
call :build_qmake "Build_qtsingleapp_%build_arch%" "%~dp0QtSingleApp\qtsingleapp\qtsingleapp\qtsingleapp.qc.pro" "%~dp0bin\%build_arch%\Release\qtsingleapp.dll" || exit /b 11
call :build_qmake "Build_MiscHelpers_%build_arch%" "%~dp0MiscHelpers\MiscHelpers.qc.pro" "%~dp0bin\%build_arch%\Release\MiscHelpers.dll" || exit /b 12
call :build_qmake "Build_QSbieAPI_%build_arch%" "%~dp0QSbieAPI\QSbieAPI.qc.pro" "%~dp0bin\%build_arch%\Release\QSbieAPI.dll" || exit /b 13
IF NOT "%build_arch%"=="x64" GOTO :after_page_host_tests

cd /d %~dp0
if exist "%~dp0Build_M3PageNavigationHostTests_%build_arch%" rmdir /S /Q "%~dp0Build_M3PageNavigationHostTests_%build_arch%"
if exist "%~dp0Build_M3PageNavigationHostTests_%build_arch%" goto :error
mkdir "%~dp0Build_M3PageNavigationHostTests_%build_arch%"
IF %ERRORLEVEL% NEQ 0 goto :error
cd /d "%~dp0Build_M3PageNavigationHostTests_%build_arch%"

%qt_path%\bin\qmake.exe %~dp0\SandMan\Tests\M3PageNavigationHostTests.pro %qt_params%
IF %ERRORLEVEL% NEQ 0 goto :error
%~dp0..\..\Qt\Tools\QtCreator\bin\jom.exe -f Makefile.Release -j 8
IF %ERRORLEVEL% NEQ 0 goto :error
if NOT EXIST release\M3PageNavigationHostTests.exe goto :error

setlocal
if "%qt_version:~0,1%"=="5" set "qt_core_dll=Qt5Core.dll"
if "%qt_version:~0,1%"=="6" set "qt_core_dll=Qt6Core.dll"
if not exist "%qt_path%\bin\%qt_core_dll%" (
  echo Missing Qt runtime: %qt_path%\bin\%qt_core_dll%
  endlocal
  goto :error
)
if not exist "%qt_path%\plugins\platforms\qoffscreen.dll" (
  echo Missing Qt offscreen platform plugin: %qt_path%\plugins\platforms\qoffscreen.dll
  endlocal
  goto :error
)
if not exist "%~dp0bin\%build_arch%\Release\MiscHelpers.dll" (
  echo Missing MiscHelpers runtime: %~dp0bin\%build_arch%\Release\MiscHelpers.dll
  endlocal
  goto :error
)
set "PATH=%qt_path%\bin;%~dp0bin\%build_arch%\Release;%PATH%"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%qt_path%\plugins\platforms"
set "QT_QPA_PLATFORM=offscreen"
where "%qt_core_dll%" >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
  echo Qt runtime is not resolvable from the run-scoped PATH
  endlocal
  goto :error
)
if exist M3PageNavigationHostTests.txt del /F /Q M3PageNavigationHostTests.txt
release\M3PageNavigationHostTests.exe -o M3PageNavigationHostTests.txt,txt
IF %ERRORLEVEL% NEQ 0 (
  type M3PageNavigationHostTests.txt
  endlocal
  goto :error
)
type M3PageNavigationHostTests.txt
endlocal

:after_page_host_tests

if "%qt_version:~0,1%"=="6" (
  set "sandman_project=%~dp0SandMan\SandMan-Qt6.qc.pro"
) else (
  set "sandman_project=%~dp0SandMan\SandMan.qc.pro"
)
call :build_qmake "Build_SandMan_%build_arch%" "%sandman_project%" "%~dp0bin\%build_arch%\Release\SandMan.exe" || exit /b 14

echo [qt] Complete for %build_arch%.
exit /b 0

:load_vs_environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [qt] Missing Visual Studio discovery tool "%VSWHERE%".
  exit /b 1
)
set "VS_INSTALLATION="
for /F "usebackq delims=" %%V in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if not defined VS_INSTALLATION set "VS_INSTALLATION=%%V"
if not defined VS_INSTALLATION (
  echo [qt] No compatible Visual Studio 2022 C++ toolchain was discovered.
  exit /b 1
)
set "VCVARSALL=%VS_INSTALLATION%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARSALL%" (
  echo [qt] Missing vcvarsall.bat at "%VCVARSALL%".
  exit /b 1
)
call "%VCVARSALL%" %vcvars_arg%
if errorlevel 1 exit /b %errorlevel%
set "SBIE_TOOLCHAIN_READY=1"
exit /b 0

:build_qmake
set "build_dir=%~dp0%~1"
if not exist "%build_dir%\." mkdir "%build_dir%"
if errorlevel 1 exit /b 1
pushd "%build_dir%"
"%qt_path%\bin\qmake.exe" "%~2" %qt_params%
if errorlevel 1 (
  popd
  exit /b 1
)
if /I "%SBIE_CLEAN_BUILD%"=="1" (
  "%jom%" -f Makefile.Release clean
  if errorlevel 1 (
    popd
    exit /b 1
  )
)
"%jom%" -f Makefile.Release -j 8
if errorlevel 1 (
  popd
  exit /b 1
)
popd
if not exist "%~3" (
  echo [qt] Expected output is missing: "%~3".
  exit /b 1
)
exit /b 0

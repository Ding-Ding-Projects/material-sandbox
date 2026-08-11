@echo off
setlocal EnableExtensions

call "%~dp0..\Installer\buildVariables.cmd" %*
if errorlevel 1 exit /b %errorlevel%
@echo off

if not defined SBIE_QT_ROOT set "SBIE_QT_ROOT=%~dp0..\Qt"
if not defined SBIE_7ZIP_EXE set "SBIE_7ZIP_EXE=C:\Program Files\7-Zip\7z.exe"
if not exist "%SBIE_7ZIP_EXE%" (
  echo [qt] Missing 7-Zip at "%SBIE_7ZIP_EXE%".
  exit /b 2
)

if /I "%~1"=="x64" goto install_x64
if /I "%~1"=="Win32" goto install_x86

echo [qt] Unsupported architecture "%~1".
exit /b 2

:install_x64
call :install_archive "msvc2022_64" "qt-everywhere-%qt_version%-Windows_7-MSVC2022-x86_64.7z" "%ghQtBuilds_hash_x64%"
exit /b %errorlevel%

:install_x86
call :install_archive "msvc2022" "qt-everywhere-%qt_version%-Windows_7-MSVC2022-x86.7z" "%ghQtBuilds_hash_x86%"
exit /b %errorlevel%

:install_archive
set "QT_ARCH=%~1"
set "QT_ARCHIVE_NAME=%~2"
set "QT_ARCHIVE_HASH=%~3"
set "QT_QMAKE=%SBIE_QT_ROOT%\%qt_version%\%QT_ARCH%\bin\qmake.exe"
if exist "%QT_QMAKE%" exit /b 0
if not defined QT_ARCHIVE_HASH (
  echo [qt] No pinned SHA-256 is configured for %QT_ARCH%.
  exit /b 3
)

set "QT_DOWNLOAD_ROOT=%SBIE_QT_ROOT%\.downloads"
set "QT_ARCHIVE=%QT_DOWNLOAD_ROOT%\%QT_ARCHIVE_NAME%"
if not exist "%QT_DOWNLOAD_ROOT%\." mkdir "%QT_DOWNLOAD_ROOT%" || exit /b 4
curl.exe --fail --location --silent --show-error "https://github.com/%ghQtBuilds_user%/%ghQtBuilds_repo%/releases/download/v%qt_version%-ssl-lgpl/%QT_ARCHIVE_NAME%" --output "%QT_ARCHIVE%"
if errorlevel 1 exit /b %errorlevel%
set "QT_ACTUAL_HASH="
for /F "tokens=1" %%H in ('certutil -hashfile "%QT_ARCHIVE%" SHA256 ^| findstr /R /I "^[0-9a-f][0-9a-f]*$"') do if not defined QT_ACTUAL_HASH set "QT_ACTUAL_HASH=%%H"
if /I not "%QT_ACTUAL_HASH%"=="%QT_ARCHIVE_HASH%" (
  echo [qt] SHA-256 mismatch for "%QT_ARCHIVE%".
  exit /b 5
)

"%SBIE_7ZIP_EXE%" x -y "-o%SBIE_QT_ROOT%" "%QT_ARCHIVE%"
if errorlevel 1 exit /b %errorlevel%
if not exist "%QT_QMAKE%" (
  echo [qt] Extraction completed without producing "%QT_QMAKE%".
  exit /b 6
)
exit /b 0

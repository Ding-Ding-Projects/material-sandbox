@echo off
setlocal EnableExtensions

if not defined SBIE_QT_ROOT set "SBIE_QT_ROOT=%~dp0..\Qt"
if not defined SBIE_7ZIP_EXE set "SBIE_7ZIP_EXE=C:\Program Files\7-Zip\7z.exe"
set "JOM_EXE=%SBIE_QT_ROOT%\Tools\QtCreator\bin\jom.exe"
set "JOM_ARCHIVE=%SBIE_QT_ROOT%\.downloads\jom_1_1_4.zip"
set "JOM_SHA256=d533c1ef49214229681e90196ed2094691e8c4a0a0bef0b2c901debcb562682b"

if exist "%JOM_EXE%" exit /b 0
if not exist "%SBIE_7ZIP_EXE%" (
  echo [jom] Missing 7-Zip at "%SBIE_7ZIP_EXE%".
  exit /b 2
)
if not exist "%SBIE_QT_ROOT%\.downloads\." mkdir "%SBIE_QT_ROOT%\.downloads" || exit /b 3

curl.exe --fail --location --silent --show-error "https://download.qt.io/official_releases/jom/jom_1_1_4.zip" --output "%JOM_ARCHIVE%"
if errorlevel 1 exit /b %errorlevel%
set "JOM_ACTUAL_HASH="
for /F "tokens=1" %%H in ('certutil -hashfile "%JOM_ARCHIVE%" SHA256 ^| findstr /R /I "^[0-9a-f][0-9a-f]*$"') do if not defined JOM_ACTUAL_HASH set "JOM_ACTUAL_HASH=%%H"
if /I not "%JOM_ACTUAL_HASH%"=="%JOM_SHA256%" (
  echo [jom] SHA-256 mismatch for "%JOM_ARCHIVE%".
  exit /b 4
)

if not exist "%SBIE_QT_ROOT%\Tools\QtCreator\bin\." mkdir "%SBIE_QT_ROOT%\Tools\QtCreator\bin" || exit /b 5
"%SBIE_7ZIP_EXE%" x -y "-o%SBIE_QT_ROOT%\Tools\QtCreator\bin" "%JOM_ARCHIVE%"
if errorlevel 1 exit /b %errorlevel%
if not exist "%JOM_EXE%" (
  echo [jom] Extraction completed without producing "%JOM_EXE%".
  exit /b 6
)
exit /b 0

@echo off
setlocal
set "ROOT=%~dp0"
set "BUILD=%LOCALAPPDATA%\ConcaGymCppBuild"
set "QT_BIN=C:\Qt\6.10.2\mingw_64\bin"
set "CONCA_GYM_PROJECT_DIR=%ROOT%"

if not exist "%BUILD%\ConcaGymCpp.exe" (
    call "%ROOT%build_cpp_qt.bat"
    if errorlevel 1 (
        echo.
        echo No se pudo compilar la app C++/Qt.
        pause
        exit /b 1
    )
)

if exist "%QT_BIN%" (
    set "PATH=%QT_BIN%;%PATH%"
)

if exist "%ROOT%.env" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%ROOT%.env") do (
        if /I "%%A"=="GEMINI_API_KEY" set "GEMINI_API_KEY=%%B"
        if /I "%%A"=="GOOGLE_API_KEY" set "GOOGLE_API_KEY=%%B"
    )
)

echo Abriendo Conca Gym C++/Qt...
"%BUILD%\ConcaGymCpp.exe"
set "APP_EXIT=%ERRORLEVEL%"

if not "%APP_EXIT%"=="0" (
    echo.
    echo La app se cerro con codigo %APP_EXIT%.
    echo Si aparece un error de Qt, revisa que exista: %QT_BIN%
    pause
)

exit /b %APP_EXIT%

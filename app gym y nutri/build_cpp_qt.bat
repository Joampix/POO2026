@echo off
setlocal
set "ROOT=%~dp0"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "NINJA=C:\Qt\Tools\Ninja\ninja.exe"
set "QT_PREFIX=C:\Qt\6.10.2\mingw_64"
set "CXX=C:\Qt\Tools\mingw1310_64\bin\g++.exe"
set "CC=C:\Qt\Tools\mingw1310_64\bin\gcc.exe"
set "BUILD=%LOCALAPPDATA%\ConcaGymCppBuild"

if not exist "%CMAKE%" (
    echo No se encontro CMake en %CMAKE%
    echo Abri cpp_qt_app\CMakeLists.txt con Qt Creator o revisa la instalacion de Qt.
    exit /b 1
)

"%CMAKE%" -S "%ROOT%cpp_qt_app" -B "%BUILD%" -G Ninja -DCMAKE_PREFIX_PATH="%QT_PREFIX%" -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_CXX_COMPILER="%CXX%" -DCMAKE_C_COMPILER="%CC%"
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%BUILD%" --config Release
if errorlevel 1 exit /b 1

echo.
echo Build listo:
echo %BUILD%\ConcaGymCpp.exe

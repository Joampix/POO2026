@echo off
setlocal
set "ROOT=%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%configurar_gemini.ps1" -Forzar
if errorlevel 1 (
    echo.
    echo No se pudo configurar Gemini. Copia la API key completa desde Google AI Studio e intenta de nuevo.
    pause
    exit /b 1
)

echo.
echo Gemini quedo configurado. Ya podes abrir la app con run_cpp_qt.bat
pause

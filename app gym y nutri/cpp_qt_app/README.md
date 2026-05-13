# Conca Gym C++ / Qt

Version C++ de la aplicacion Conca Gym, usando Qt Widgets y Qt Network.

## Funciones incluidas

- Perfil de usuario.
- Login inicial contra FastAPI con fallback offline.
- Auditoria local SQLite para intentos de login, ultimo login y eventos menores.
- Calculadora de calorias y macros.
- Plan nutricional educativo.
- Rutina segun objetivo, nivel, dias y equipamiento.
- Referencias visuales de ejercicios desde ExerciseDB OSS.
- Fichas complementarias de ejercicios desde wger API.
- Recetas locales ajustadas a macros e ingredientes disponibles.
- Progreso semanal guardado en JSON.
- Coach IA con Gemini usando `GEMINI_API_KEY` desde Windows o `.env`.
- Informacion cientifica y aviso educativo.

## Requisitos

- Qt 6 con modulos Widgets, Network y Sql.
- CMake 3.21 o superior.
- Compilador C++17:
  - MSVC en Windows, o
  - MinGW compatible con tu instalacion de Qt.

## Compilar con Qt Creator

1. Abrir Qt Creator.
2. `File > Open File or Project`.
3. Elegir `cpp_qt_app/CMakeLists.txt`.
4. Seleccionar un kit Qt 6.
5. Build y Run.

## Compilar desde terminal

En esta PC Qt esta instalado en `C:\Qt`, pero CMake no esta en PATH. Desde la carpeta raiz del proyecto podes usar:

```powershell
.\build_cpp_qt.bat
.\run_cpp_qt.bat
```

El build se genera en `%LOCALAPPDATA%\ConcaGymCppBuild` para evitar bloqueos de OneDrive.

Tambien se puede compilar manualmente:

```powershell
cd cpp_qt_app
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ConcaGymCpp.exe
```

Si `cmake` no aparece en PowerShell, abrir el proyecto con Qt Creator o usar los `.bat` de la raiz.

## APIs

- ExerciseDB OSS: no requiere API key.
- wger API: endpoints publicos de lectura, no requiere API key para ejercicios.
- Gemini API: requiere `GEMINI_API_KEY`.
- FastAPI propia: `CONCA_GYM_API_URL`, por defecto `http://127.0.0.1:8000`.

Para configurar Gemini una sola vez desde la raiz del proyecto:

```powershell
.\configurar_gemini.ps1
```

La clave queda fuera del codigo fuente. Si falta, la app muestra `Gemini sin clave` en la seccion Coach IA.

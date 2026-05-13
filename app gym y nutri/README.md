# Conca Gym C++/Qt

Aplicacion de escritorio para entrenamiento, nutricion, recetas, progreso semanal y Coach IA.

## Ejecutar

Primero, si queres login real con servidor, levanta el backend:

```powershell
cd backend
docker compose up -d --build
cd ..
```

FastAPI queda en `http://localhost:8000` y phpMyAdmin en `http://localhost:8081`.

Luego ejecuta la app Qt:

```powershell
.\run_cpp_qt.bat
```

Si hace falta recompilar:

```powershell
.\build_cpp_qt.bat
```

El ejecutable queda en:

```text
%LOCALAPPDATA%\ConcaGymCppBuild\ConcaGymCpp.exe
```

## Proyecto C++

Abrir en Qt Creator:

```text
cpp_qt_app/CMakeLists.txt
```

## Gemini

El Coach IA usa Gemini API. Para configurar la clave una sola vez:

```powershell
.\configurar_gemini.ps1
```

Tambien podes hacer doble click en:

```text
configurar_gemini.bat
```

La clave se guarda en `.env`, que esta ignorado por `.gitignore`. Despues la app la usa automaticamente cada vez que abre.

## APIs usadas

- FastAPI propia para login y registro de eventos.
- MySQL propia para usuarios/intentos/eventos del servidor.
- SQLite local para intentos fallidos, ultimo login exitoso y eventos menores.
- ExerciseDB OSS para referencias visuales de ejercicios.
- wger API para fichas complementarias de ejercicios, musculos, equipo e imagenes.
- Gemini API para el Coach IA contextual.

## Aviso

La app es educativa y no reemplaza a un medico, nutricionista ni entrenador certificado.

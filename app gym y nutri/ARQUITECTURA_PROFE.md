# Arquitectura pedida por el profesor

## Jose

- MySQL en Docker Compose.
- phpMyAdmin en Docker Compose.
- Base de datos del servidor para usuarios, intentos de login y eventos.
- SQLite local en Qt para:
  - intentos fallidos locales;
  - ultimo login exitoso;
  - eventos menores locales.

Archivos:

- `backend/docker-compose.yml`
- `backend/app/models.py`
- `cpp_qt_app/src/LocalAuditStore.h`
- `cpp_qt_app/src/LocalAuditStore.cpp`

## Facundo

- FastAPI en Docker Compose.
- Preparado para desplegar en Contabo u otro VPS con Docker.
- Clase `DataManager` en Qt para comunicarse con FastAPI.
- Las APIs de terceros se consultan directo desde Qt:
  - ExerciseDB;
  - wger;
  - Gemini.
- Qt consulta FastAPI para:
  - login;
  - registrar eventos de la aplicacion.

Archivos:

- `backend/app/main.py`
- `backend/Dockerfile`
- `cpp_qt_app/src/DataManager.h`
- `cpp_qt_app/src/DataManager.cpp`
- `cpp_qt_app/src/ExerciseDbClient.cpp`
- `cpp_qt_app/src/WgerClient.cpp`
- `cpp_qt_app/src/GeminiClient.cpp`

## Juampi

- Interfaz Qt orientada a usuario final.
- Login inicial.
- Navegacion lateral clara.
- Secciones separadas:
  - perfil;
  - plan nutricional;
  - entrenamiento;
  - recetas;
  - progreso;
  - Coach IA;
  - informacion cientifica.

Archivos:

- `cpp_qt_app/src/LoginDialog.h`
- `cpp_qt_app/src/LoginDialog.cpp`
- `cpp_qt_app/src/MainWindow.h`
- `cpp_qt_app/src/MainWindow.cpp`

## Flujo principal

1. Usuario abre la app Qt.
2. Qt muestra login.
3. Qt llama a FastAPI `POST /auth/login`.
4. FastAPI valida contra MySQL.
5. Si es correcto, devuelve token y guarda ultimo login en MySQL.
6. Qt guarda ultimo login tambien en SQLite local.
7. Qt registra eventos importantes en FastAPI `POST /events`.
8. Qt guarda eventos menores en SQLite local.
9. APIs de terceros se consultan directo desde Qt.

## Comandos utiles

Backend:

```powershell
cd backend
docker compose up -d --build
```

App Qt:

```powershell
.\build_cpp_qt.bat
.\run_cpp_qt.bat
```

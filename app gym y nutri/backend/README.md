# Backend Conca Gym

Backend pedido para la entrega: FastAPI + MySQL + phpMyAdmin en Docker.

## Levantar servicios

Desde esta carpeta:

```powershell
docker compose up -d --build
```

Servicios:

- FastAPI: http://localhost:8000
- Docs Swagger: http://localhost:8000/docs
- phpMyAdmin: http://localhost:8081
- MySQL: localhost:3306

Usuario inicial:

```text
usuario: admin
password: admin123
```

Cambiar esos valores en `docker-compose.yml` antes de subir a un servidor real.

## Endpoints principales

- `GET /health`
- `POST /auth/login`
- `POST /events`
- `GET /events`

Qt usa FastAPI para login y eventos. Las APIs de terceros como ExerciseDB, wger y Gemini se consultan directo desde Qt.

import os
from datetime import datetime

from fastapi import Depends, FastAPI, Header, HTTPException, Request, status
from sqlalchemy import select
from sqlalchemy.orm import Session

from .database import Base, engine, get_db
from .models import AppEvent, LoginAttempt, User
from .schemas import EventCreate, EventResponse, HealthResponse, LoginRequest, LoginResponse
from .security import create_token, hash_password, verify_password, verify_token


app = FastAPI(title="Conca Gym API", version="0.1.0")


@app.on_event("startup")
def startup() -> None:
    Base.metadata.create_all(bind=engine)
    with Session(engine) as db:
        username = os.getenv("DEFAULT_ADMIN_USERNAME", "admin")
        existing = db.scalar(select(User).where(User.username == username))
        if existing:
            return
        user = User(
            username=username,
            password_hash=hash_password(os.getenv("DEFAULT_ADMIN_PASSWORD", "admin123")),
            full_name=os.getenv("DEFAULT_ADMIN_FULL_NAME", "Administrador"),
            role="admin",
            is_active=True,
        )
        db.add(user)
        db.commit()


def client_ip(request: Request) -> str:
    forwarded = request.headers.get("x-forwarded-for")
    if forwarded:
        return forwarded.split(",")[0].strip()
    return request.client.host if request.client else ""


def record_event(
    db: Session,
    event_type: str,
    username: str = "",
    user_id: int | None = None,
    severity: str = "info",
    source: str = "api",
    detail: str = "",
) -> AppEvent:
    event = AppEvent(
        user_id=user_id,
        username=username,
        event_type=event_type,
        severity=severity,
        source=source,
        detail=detail,
    )
    db.add(event)
    return event


def current_user(
    authorization: str | None = Header(default=None),
    db: Session = Depends(get_db),
) -> User:
    if not authorization or not authorization.lower().startswith("bearer "):
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Falta token")
    username = verify_token(authorization.split(" ", 1)[1].strip())
    if not username:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token invalido")
    user = db.scalar(select(User).where(User.username == username, User.is_active.is_(True)))
    if not user:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Usuario invalido")
    return user


@app.get("/health", response_model=HealthResponse)
def health() -> HealthResponse:
    return HealthResponse(status="ok", service="conca-gym-api")


@app.post("/auth/login", response_model=LoginResponse)
def login(payload: LoginRequest, request: Request, db: Session = Depends(get_db)) -> LoginResponse:
    username = payload.username.strip().lower()
    user_agent = request.headers.get("user-agent", "")
    user = db.scalar(select(User).where(User.username == username))

    success = bool(user and user.is_active and verify_password(payload.password, user.password_hash))
    attempt = LoginAttempt(
        username=username,
        success=success,
        reason="" if success else "credenciales invalidas",
        ip_address=client_ip(request),
        user_agent=user_agent[:255],
    )
    db.add(attempt)

    if not success or not user:
        record_event(
            db,
            event_type="auth.login_failed",
            username=username,
            severity="warning",
            source="fastapi",
            detail="Intento de login fallido",
        )
        db.commit()
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Usuario o password incorrecto")

    user.last_login_at = datetime.utcnow()
    record_event(
        db,
        event_type="auth.login_success",
        username=user.username,
        user_id=user.id,
        severity="info",
        source="fastapi",
        detail="Login exitoso desde Qt",
    )
    db.commit()
    db.refresh(user)

    return LoginResponse(
        token=create_token(user.username),
        username=user.username,
        full_name=user.full_name,
        role=user.role,
        last_login_at=user.last_login_at,
    )


@app.post("/events", response_model=EventResponse)
def create_event(
    payload: EventCreate,
    db: Session = Depends(get_db),
    user: User = Depends(current_user),
) -> EventResponse:
    event = record_event(
        db,
        event_type=payload.event_type,
        username=user.username,
        user_id=user.id,
        severity=payload.severity,
        source=payload.source,
        detail=payload.detail,
    )
    db.commit()
    db.refresh(event)
    return EventResponse(
        id=event.id,
        username=event.username,
        event_type=event.event_type,
        severity=event.severity,
        source=event.source,
        detail=event.detail,
        created_at=event.created_at,
    )


@app.get("/events", response_model=list[EventResponse])
def list_events(
    limit: int = 100,
    db: Session = Depends(get_db),
    user: User = Depends(current_user),
) -> list[EventResponse]:
    safe_limit = max(1, min(limit, 200))
    rows = db.scalars(
        select(AppEvent)
        .where(AppEvent.user_id == user.id)
        .order_by(AppEvent.created_at.desc())
        .limit(safe_limit)
    ).all()
    return [
        EventResponse(
            id=row.id,
            username=row.username,
            event_type=row.event_type,
            severity=row.severity,
            source=row.source,
            detail=row.detail,
            created_at=row.created_at,
        )
        for row in rows
    ]

from datetime import datetime

from pydantic import BaseModel, Field


class HealthResponse(BaseModel):
    status: str
    service: str


class LoginRequest(BaseModel):
    username: str = Field(min_length=1, max_length=80)
    password: str = Field(min_length=1, max_length=255)


class LoginResponse(BaseModel):
    token: str
    username: str
    full_name: str
    role: str
    last_login_at: datetime | None


class EventCreate(BaseModel):
    event_type: str = Field(min_length=1, max_length=80)
    severity: str = Field(default="info", max_length=20)
    source: str = Field(default="qt", max_length=60)
    detail: str = Field(default="", max_length=4000)


class EventResponse(BaseModel):
    id: int
    username: str
    event_type: str
    severity: str
    source: str
    detail: str
    created_at: datetime

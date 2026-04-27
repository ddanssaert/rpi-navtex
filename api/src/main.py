from contextlib import asynccontextmanager
from fastapi import FastAPI, Depends, status, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.ext.asyncio import AsyncSession
from .database import engine, get_db, Base
from .models import Message
from .schemas import MessageCreate, MessageRead
from .broadcast import manager
from .config_schema import SDRConfig
from .security import ensure_certs
from fastapi.responses import FileResponse
import json
import os
import httpx
import logging

logger = logging.getLogger("api.config")
SDR_CONTROL_URL_DEFAULT = "http://sdr-dsp:8001/control/config"

CONFIG_FILE = os.getenv("CONFIG_FILE", "/data/config.json")

def load_config():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            return SDRConfig(**json.load(f))
    return SDRConfig()

def save_config(config: SDRConfig):
    with open(CONFIG_FILE, "w") as f:
        json.dump(config.model_dump(), f)

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Ensure SSL certificates are ready
    ensure_certs()
    yield

app = FastAPI(lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # For production, replace with specific origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def read_root():
    return {"message": "Navtex API Broker is running. Visit port 80 for the PWA dashboard."}

@app.get("/health")
def health_check():
    return {"status": "ok"}

@app.get("/certs/ca.crt")
async def download_ca():
    ca_path = "/data/certs/rootCA.crt"
    if os.path.exists(ca_path):
        return FileResponse(
            ca_path, 
            media_type="application/x-x509-ca-cert",
            filename="navtex-root-ca.crt"
        )
    return {"error": "CA certificate not found"}

@app.post("/messages", response_model=MessageRead, status_code=status.HTTP_201_CREATED)
async def create_message(message: MessageCreate, db: AsyncSession = Depends(get_db)):
    db_message = Message(**message.model_dump())
    db.add(db_message)
    await db.commit()
    await db.refresh(db_message)
    
    # Broadcast to websocket clients
    await manager.broadcast(MessageRead.model_validate(db_message).model_dump(mode="json"))
    
    return db_message

@app.get("/messages", response_model=list[MessageRead])
async def get_messages(db: AsyncSession = Depends(get_db)):
    from sqlalchemy.future import select
    result = await db.execute(select(Message).order_by(Message.timestamp.desc()))
    return result.scalars().all()

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)

@app.get("/config", response_model=SDRConfig)
async def get_config():
    return load_config()

async def _notify_sdr_dsp(payload: dict) -> None:
    """Forward config change to sdr-dsp control endpoint.
    Failures are swallowed: config.json on the shared volume is the
    authoritative fallback that sdr-dsp reads at startup.
    """
    url = os.getenv("SDR_CONTROL_URL", SDR_CONTROL_URL_DEFAULT)
    try:
        async with httpx.AsyncClient(timeout=3.0) as client:
            resp = await client.post(url, json=payload)
            if resp.status_code >= 400:
                logger.warning(
                    "sdr-dsp control endpoint returned %d: %s",
                    resp.status_code, resp.text[:200])
    except httpx.RequestError as e:
        logger.warning("sdr-dsp control endpoint unreachable (%s): %s",
                       type(e).__name__, e)

@app.post("/config", response_model=SDRConfig)
async def update_config(config: SDRConfig):
    save_config(config)
    await _notify_sdr_dsp(config.model_dump())
    return config

@app.post("/test-notify")
async def trigger_test_notification():
    await manager.broadcast({
        "type": "test_notification",
        "title": "NAVTEX Test",
        "body": "This is a broadcast test notification from the RPi Navtex receiver."
    })
    return {"status": "notification_triggered"}

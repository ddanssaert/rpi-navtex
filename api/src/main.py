from contextlib import asynccontextmanager
from fastapi import FastAPI, Depends, status, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.ext.asyncio import AsyncSession
from .database import engine, get_db, Base
from .models import Message, PushSubscription
from .schemas import MessageCreate, MessageRead, PushSubscriptionCreate, PushUnsubscribe, PushFiltersUpdate
from .broadcast import manager
from .config_schema import SDRConfig
from .security import ensure_certs
from .push_utils import load_or_generate_vapid_keys, send_push_notification
from fastapi.responses import FileResponse
import json
import os
import httpx
import logging

logger = logging.getLogger("api.config")
SDR_CONTROL_URL_DEFAULT = "http://sdr-dsp:8001/control/config"

CONFIG_FILE = os.getenv("CONFIG_FILE", "/data/config.json")


def _matches_filters(sub_filters, station_id: str, message_type: str) -> bool:
    if sub_filters is None:
        return True
    stations = sub_filters.get("stations", [])
    types = sub_filters.get("types", [])
    return (len(stations) == 0 or station_id in stations) and \
           (len(types) == 0 or message_type in types)

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
    # Ensure VAPID keys are ready
    load_or_generate_vapid_keys()
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
    
    # Send Web Push notifications
    from .push_utils import load_or_generate_vapid_keys, send_push_notification
    from .models import PushSubscription
    from sqlalchemy.future import select
    from sqlalchemy import delete
    
    vapid_keys = load_or_generate_vapid_keys()
    result = await db.execute(select(PushSubscription))
    subscriptions = result.scalars().all()
    
    push_data = {
        "title": "New NAVTEX Message",
        "station_id": db_message.station_id,
        "message_type": db_message.message_type,
        "content": db_message.content[:100]
    }
    
    for sub in subscriptions:
        if not _matches_filters(sub.filters, db_message.station_id, db_message.message_type):
            continue
        sub_info = {
            "endpoint": sub.endpoint,
            "keys": {
                "p256dh": sub.p256dh,
                "auth": sub.auth
            }
        }
        success = send_push_notification(sub_info, push_data, vapid_keys)
        if not success:
            # Subscription expired, remove it
            await db.execute(delete(PushSubscription).where(PushSubscription.id == sub.id))
    
    await db.commit()
    
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
async def trigger_test_notification(db: AsyncSession = Depends(get_db)):
    # WebSocket broadcast
    await manager.broadcast({
        "type": "test_notification",
        "title": "NAVTEX Test",
        "body": "This is a broadcast test notification from the RPi Navtex receiver."
    })
    
    # Web Push broadcast
    from .push_utils import load_or_generate_vapid_keys, send_push_notification
    from .models import PushSubscription
    from sqlalchemy.future import select
    
    vapid_keys = load_or_generate_vapid_keys()
    result = await db.execute(select(PushSubscription))
    subscriptions = result.scalars().all()
    
    push_data = {
        "title": "NAVTEX Broadcast Test",
        "content": "This is a test notification sent to all registered devices.",
        "station_id": "TEST",
        "message_type": "Z"
    }
    
    for sub in subscriptions:
        sub_info = {
            "endpoint": sub.endpoint,
            "keys": {
                "p256dh": sub.p256dh,
                "auth": sub.auth
            }
        }
        send_push_notification(sub_info, push_data, vapid_keys)
        # We don't prune on test notify to keep it simple, or we could.
        
    return {"status": "notification_triggered", "subscribers_notified": len(subscriptions)}

@app.get("/push/vapid-key")
async def get_vapid_key():
    keys = load_or_generate_vapid_keys()
    return {"public_key": keys["public_key"]}

@app.post("/push/subscribe")
async def subscribe(sub: PushSubscriptionCreate, db: AsyncSession = Depends(get_db)):
    from sqlalchemy.future import select
    # Check if already exists
    result = await db.execute(select(PushSubscription).where(PushSubscription.endpoint == sub.endpoint))
    existing = result.scalars().first()
    
    if existing:
        existing.p256dh = sub.keys.p256dh
        existing.auth = sub.keys.auth
        if sub.filters is not None:
            existing.filters = sub.filters.model_dump()
    else:
        db_sub = PushSubscription(
            endpoint=sub.endpoint,
            p256dh=sub.keys.p256dh,
            auth=sub.keys.auth,
            filters=sub.filters.model_dump() if sub.filters else None
        )
        db.add(db_sub)

    await db.commit()
    return {"status": "subscribed"}

@app.put("/push/filters")
async def update_push_filters(update: PushFiltersUpdate, db: AsyncSession = Depends(get_db)):
    from sqlalchemy.future import select
    from fastapi import HTTPException
    result = await db.execute(
        select(PushSubscription).where(PushSubscription.endpoint == update.endpoint)
    )
    existing = result.scalars().first()
    if not existing:
        raise HTTPException(status_code=404, detail="Subscription not found")
    existing.filters = update.filters.model_dump()
    await db.commit()
    return {"status": "updated"}

@app.delete("/push/unsubscribe")
async def unsubscribe(unsub: PushUnsubscribe, db: AsyncSession = Depends(get_db)):
    from sqlalchemy import delete
    await db.execute(delete(PushSubscription).where(PushSubscription.endpoint == unsub.endpoint))
    await db.commit()
    return {"status": "unsubscribed"}

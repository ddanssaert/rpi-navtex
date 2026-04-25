from contextlib import asynccontextmanager
from fastapi import FastAPI, Depends, status, WebSocket, WebSocketDisconnect
from sqlalchemy.ext.asyncio import AsyncSession
from .database import engine, get_db, Base
from .models import Message
from .schemas import MessageCreate, MessageRead
from .broadcast import manager

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Create tables
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
    yield

app = FastAPI(lifespan=lifespan)

@app.get("/health")
def health_check():
    return {"status": "ok"}

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

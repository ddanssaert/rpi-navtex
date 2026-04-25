from contextlib import asynccontextmanager
from fastapi import FastAPI, Depends, status
from sqlalchemy.ext.asyncio import AsyncSession
from .database import engine, get_db, Base
from .models import Message
from .schemas import MessageCreate, MessageRead

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
    return db_message

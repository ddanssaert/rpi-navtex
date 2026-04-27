from sqlalchemy import Column, Integer, String, Text, DateTime, JSON
from sqlalchemy.sql import func
from .database import Base

class Message(Base):
    __tablename__ = "messages"

    id = Column(Integer, primary_key=True, index=True)
    station_id = Column(String(1), nullable=False)
    message_type = Column(String(1), nullable=False)
    serial_id = Column(Integer, nullable=False)
    content = Column(Text, nullable=False)
    raw_data = Column(Text, nullable=False)
    timestamp = Column(DateTime(timezone=True), server_default=func.now())

class PushSubscription(Base):
    __tablename__ = "push_subscriptions"

    id = Column(Integer, primary_key=True, index=True)
    endpoint = Column(String, unique=True, nullable=False)
    p256dh = Column(String, nullable=False)
    auth = Column(String, nullable=False)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    filters = Column(JSON, nullable=True)

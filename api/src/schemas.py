from pydantic import BaseModel, ConfigDict
from datetime import datetime
from typing import List, Optional

class MessageBase(BaseModel):
    station_id: str
    message_type: str
    serial_id: int
    content: str
    raw_data: str

class MessageCreate(MessageBase):
    pass

class MessageRead(MessageBase):
    model_config = ConfigDict(from_attributes=True)
    
    id: int
    timestamp: datetime

class PushSubscriptionKeys(BaseModel):
    p256dh: str
    auth: str

class PushFilters(BaseModel):
    stations: List[str] = []
    types: List[str] = []

class PushSubscriptionCreate(BaseModel):
    endpoint: str
    keys: PushSubscriptionKeys
    filters: Optional[PushFilters] = None

class PushFiltersUpdate(BaseModel):
    endpoint: str
    filters: PushFilters

class PushUnsubscribe(BaseModel):
    endpoint: str

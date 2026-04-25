from pydantic import BaseModel, ConfigDict
from datetime import datetime
from typing import Optional

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

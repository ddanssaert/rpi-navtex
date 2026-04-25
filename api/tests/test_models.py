from src.models import Message
from src.database import Base

def test_message_model_exists():
    assert Message.__tablename__ == "messages"

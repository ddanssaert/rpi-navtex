import pytest
from httpx import AsyncClient, ASGITransport
from src.main import app
from src.database import engine, Base

@pytest.mark.asyncio
async def test_post_message():
    # Ensure tables are created for the test
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
        
    data = {
        "station_id": "A",
        "message_type": "D",
        "serial_id": 1,
        "content": "SEARCH AND RESCUE",
        "raw_data": "ZCZC AD01 SEARCH AND RESCUE NNNN"
    }
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.post("/messages", json=data)
    assert response.status_code == 201
    assert response.json()["station_id"] == "A"

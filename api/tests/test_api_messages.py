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

@pytest.mark.asyncio
async def test_get_messages():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.get("/messages")
    assert response.status_code == 200
    assert isinstance(response.json(), list)

@pytest.mark.asyncio
async def test_websocket_broadcast():
    from fastapi.testclient import TestClient
    with TestClient(app) as client:
        with client.websocket_connect("/ws") as websocket:
            data = {
                "station_id": "B",
                "message_type": "E",
                "serial_id": 2,
                "content": "WEATHER REPORT",
                "raw_data": "ZCZC BE02 WEATHER REPORT NNNN"
            }
            client.post("/messages", json=data)
            broadcast_data = websocket.receive_json()
            assert broadcast_data["station_id"] == "B"
            assert broadcast_data["content"] == "WEATHER REPORT"

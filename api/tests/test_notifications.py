import pytest
from httpx import AsyncClient, ASGITransport
from src.main import app
from fastapi.testclient import TestClient

@pytest.mark.asyncio
async def test_trigger_notification_endpoint():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.post("/test-notify")
    assert response.status_code == 200
    assert response.json() == {"status": "notification_triggered"}

@pytest.mark.asyncio
async def test_test_notification_broadcast():
    with TestClient(app) as client:
        with client.websocket_connect("/ws") as websocket:
            # Trigger the notification
            response = client.post("/test-notify")
            assert response.status_code == 200
            
            # Check if we received the broadcast
            broadcast_data = websocket.receive_json()
            assert broadcast_data["type"] == "test_notification"
            assert "title" in broadcast_data
            assert "body" in broadcast_data

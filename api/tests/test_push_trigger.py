import pytest
from httpx import AsyncClient, ASGITransport
from src.main import app
from src.database import SessionLocal, engine, Base
from src.models import PushSubscription, Message
import json
import responses

@pytest.mark.asyncio
async def test_push_triggered_on_message():
    # Ensure tables are created
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    # Add a subscription to DB
    endpoint = "https://fcm.googleapis.com/fcm/send/trigger-test-token"
    
    # Generate a valid-looking p256dh (uncompressed EC point)
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives import serialization
    import base64
    import secrets
    
    pk = ec.generate_private_key(ec.SECP256R1())
    pub_bytes = pk.public_key().public_bytes(
        encoding=serialization.Encoding.X962,
        format=serialization.PublicFormat.UncompressedPoint
    )
    p256dh = base64.urlsafe_b64encode(pub_bytes).decode('utf-8').rstrip('=')
    auth = base64.urlsafe_b64encode(secrets.token_bytes(16)).decode('utf-8').rstrip('=')

    async with SessionLocal() as db:
        db_sub = PushSubscription(
            endpoint=endpoint,
            p256dh=p256dh,
            auth=auth
        )
        db.add(db_sub)
        await db.commit()

    # Mock the push service response
    with responses.RequestsMock() as rsps:
        rsps.add(responses.POST, endpoint, status=201)

        data = {
            "station_id": "A",
            "message_type": "D",
            "serial_id": 1,
            "content": "GALE WARNING",
            "raw_data": "ZCZC AD01 GALE WARNING NNNN"
        }

        async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
            response = await ac.post("/messages", json=data)
            assert response.status_code == 201

        # Verify push service was called
        assert len(rsps.calls) == 1
        assert rsps.calls[0].request.url == endpoint
        assert isinstance(rsps.calls[0].request.body, bytes)

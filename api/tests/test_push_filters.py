import pytest
from httpx import AsyncClient, ASGITransport
from src.main import app, _matches_filters
from src.database import SessionLocal, engine, Base
from src.models import PushSubscription
import responses


# ── Unit tests for _matches_filters ──────────────────────────────────────────

def test_matches_filters_none_returns_true():
    assert _matches_filters(None, "A", "B") is True


def test_matches_filters_empty_dict_returns_true():
    assert _matches_filters({}, "A", "B") is True


def test_matches_filters_empty_arrays_returns_true():
    assert _matches_filters({"stations": [], "types": []}, "A", "B") is True


def test_matches_filters_station_match():
    assert _matches_filters({"stations": ["A"], "types": []}, "A", "Z") is True


def test_matches_filters_station_no_match():
    assert _matches_filters({"stations": ["A"], "types": []}, "B", "Z") is False


def test_matches_filters_type_match():
    assert _matches_filters({"stations": [], "types": ["Z"]}, "A", "Z") is True


def test_matches_filters_type_no_match():
    assert _matches_filters({"stations": [], "types": ["Z"]}, "A", "A") is False


def test_matches_filters_both_match():
    assert _matches_filters({"stations": ["A"], "types": ["Z"]}, "A", "Z") is True


def test_matches_filters_station_matches_type_no():
    assert _matches_filters({"stations": ["A"], "types": ["Z"]}, "A", "B") is False


# ── API tests ─────────────────────────────────────────────────────────────────

@pytest.mark.asyncio
async def test_put_push_filters_updates_subscription():
    async with SessionLocal() as db:
        db_sub = PushSubscription(
            endpoint="https://push.example.com/filter-test",
            p256dh="p256dh_val",
            auth="auth_val"
        )
        db.add(db_sub)
        await db.commit()

    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.put("/push/filters", json={
            "endpoint": "https://push.example.com/filter-test",
            "filters": {"stations": ["A", "B"], "types": ["Z"]}
        })

    assert response.status_code == 200
    assert response.json()["status"] == "updated"

    from sqlalchemy.future import select
    async with SessionLocal() as db:
        result = await db.execute(
            select(PushSubscription).where(
                PushSubscription.endpoint == "https://push.example.com/filter-test"
            )
        )
        sub = result.scalars().first()
        assert sub.filters == {"stations": ["A", "B"], "types": ["Z"]}


@pytest.mark.asyncio
async def test_put_push_filters_returns_404_for_unknown_endpoint():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.put("/push/filters", json={
            "endpoint": "https://push.example.com/does-not-exist",
            "filters": {"stations": [], "types": []}
        })

    assert response.status_code == 404


@pytest.mark.asyncio
async def test_subscribe_persists_filters():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
        response = await ac.post("/push/subscribe", json={
            "endpoint": "https://push.example.com/with-filters",
            "keys": {"p256dh": "p256dh_val", "auth": "auth_val"},
            "filters": {"stations": ["C"], "types": ["A", "B"]}
        })

    assert response.status_code == 200

    from sqlalchemy.future import select
    async with SessionLocal() as db:
        result = await db.execute(
            select(PushSubscription).where(
                PushSubscription.endpoint == "https://push.example.com/with-filters"
            )
        )
        sub = result.scalars().first()
        assert sub.filters == {"stations": ["C"], "types": ["A", "B"]}


@pytest.mark.asyncio
async def test_push_only_sent_to_matching_subscription():
    """Only the subscriber whose filters match station_id/message_type receives the push."""
    from unittest.mock import patch

    endpoint_match = "https://fcm.example.com/match-token"
    endpoint_no_match = "https://fcm.example.com/no-match-token"

    push_calls = []

    def mock_send(sub_info, push_data, vapid_keys):
        push_calls.append(sub_info["endpoint"])
        return True

    with patch("src.push_utils.send_push_notification", mock_send), \
         patch("src.main.send_push_notification", mock_send):
        async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as ac:
            # Register two subscribers via the API so they use the app's own DB session
            await ac.post("/push/subscribe", json={
                "endpoint": endpoint_match,
                "keys": {"p256dh": "p256dh_match", "auth": "auth_match"},
                "filters": {"stations": ["A"], "types": []}
            })
            await ac.post("/push/subscribe", json={
                "endpoint": endpoint_no_match,
                "keys": {"p256dh": "p256dh_no", "auth": "auth_no"},
                "filters": {"stations": ["B"], "types": []}
            })

            response = await ac.post("/messages", json={
                "station_id": "A",
                "message_type": "D",
                "serial_id": 1,
                "content": "GALE WARNING",
                "raw_data": "ZCZC AD01 GALE WARNING NNNN"
            })

    assert response.status_code == 201
    assert endpoint_match in push_calls
    assert endpoint_no_match not in push_calls

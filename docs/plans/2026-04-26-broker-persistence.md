# Navtex Broker & Persistence Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Implement asynchronous SQLite persistence and a WebSocket broadcast system for incoming Navtex messages.

**Architecture:** Use SQLAlchemy 2.0 with `aiosqlite` for non-blocking database operations. Incoming messages from the SDR pipeline are stored in SQLite and instantly broadcast to all connected WebSocket clients.

**Tech Stack:** Python 3.11, FastAPI, SQLAlchemy, aiosqlite, Pydantic.

---

### Task 1: Add Database Dependencies and Async Setup

**Files:**
- Modify: `api/requirements.txt`
- Create: `api/src/database.py`
- Modify: `api/src/main.py`

**Step 1: Write the failing test**

```python
# api/tests/test_db_setup.py
import pytest
from src.database import engine

@pytest.mark.asyncio
async def test_db_connection():
    async with engine.connect() as conn:
        result = await conn.execute("SELECT 1")
        assert result.scalar() == 1
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_db_setup.py`
Expected: FAIL with "ModuleNotFoundError: No module named 'sqlalchemy'"

**Step 3: Write minimal implementation**

Update `api/requirements.txt` with `sqlalchemy`, `aiosqlite`. Create `api/src/database.py` with async engine and sessionmaker.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/test_db_setup.py`
Expected: PASS

**Step 5: Commit**

```bash
git add api/requirements.txt api/src/database.py
git commit -m "feat: setup async sqlalchemy with sqlite"
```

---

### Task 2: Define Message Model and Schema

**Files:**
- Create: `api/src/models.py`
- Create: `api/src/schemas.py`
- Test: `api/tests/test_models.py`

**Step 1: Write the failing test**

```python
# api/tests/test_models.py
from src.models import Message
from src.database import Base

def test_message_model_exists():
    assert Message.__tablename__ == "messages"
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_models.py`
Expected: FAIL with "ImportError"

**Step 3: Write minimal implementation**

Define `Message` model in `models.py` and `MessageCreate`/`MessageRead` Pydantic schemas in `schemas.py`.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/test_models.py`
Expected: PASS

**Step 5: Commit**

```bash
git add api/src/models.py api/src/schemas.py api/tests/test_models.py
git commit -m "feat: define navtex message models and schemas"
```

---

### Task 3: Implement Message Reception (POST /messages)

**Files:**
- Modify: `api/src/main.py`
- Test: `api/tests/test_api_messages.py`

**Step 1: Write the failing test**

```python
# api/tests/test_api_messages.py
from fastapi.testclient import TestClient
from src.main import app

client = TestClient(app)

def test_post_message():
    data = {
        "station_id": "A",
        "message_type": "D",
        "serial_id": 1,
        "content": "SEARCH AND RESCUE",
        "raw_data": "ZCZC AD01 SEARCH AND RESCUE NNNN"
    }
    response = client.post("/messages", json=data)
    assert response.status_code == 201
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_api_messages.py`
Expected: FAIL with 404 (or 405)

**Step 3: Write minimal implementation**

Add `POST /messages` route to `main.py` that saves the message to the database.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/test_api_messages.py`
Expected: PASS

**Step 5: Commit**

```bash
git add api/src/main.py api/tests/test_api_messages.py
git commit -m "feat: implement message reception endpoint"
```

---

### Task 4: Implement Message History (GET /messages)

**Files:**
- Modify: `api/src/main.py`

**Step 1: Write the failing test**

```python
def test_get_messages():
    response = client.get("/messages")
    assert response.status_code == 200
    assert isinstance(response.json(), list)
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_api_messages.py`
Expected: FAIL with 404

**Step 3: Write minimal implementation**

Add `GET /messages` route to `main.py` with basic pagination and filtering.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/test_api_messages.py`
Expected: PASS

**Step 5: Commit**

```bash
git add api/src/main.py
git commit -m "feat: implement message history endpoint"
```

---

### Task 5: Implement Live Broadcast (WebSockets)

**Files:**
- Modify: `api/src/main.py`
- Create: `api/src/broadcast.py`

**Step 1: Write the failing test**

```python
def test_websocket_broadcast():
    with client.websocket_connect("/ws") as websocket:
        # We need a way to trigger a broadcast from the test
        app.broadcast_manager.notify({"status": "test"})
        data = websocket.receive_json()
        assert data == {"status": "test"}
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_api_messages.py`
Expected: FAIL with "AttributeError" or connection failure.

**Step 3: Write minimal implementation**

Implement a `ConnectionManager` in `broadcast.py` and a `/ws` endpoint in `main.py`. Trigger `broadcast()` inside the `POST /messages` handler.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/test_api_messages.py`
Expected: PASS

**Step 5: Commit**

```bash
git add api/src/main.py api/src/broadcast.py
git commit -m "feat: implement websocket broadcast for real-time updates"
```

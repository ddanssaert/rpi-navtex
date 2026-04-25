# Navtex Foundation Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Scaffold the exact repository structure, `docker-compose` stack, and API skeleton for the Navtex Receiver Appliance.

**Architecture:** A containerized microservices approach to serve as the appliance core. It boots three primary services: `sdr-dsp` (C++ parser), `api-broker` (Python FastAPI), and `pwa-frontend` (React). 

**Tech Stack:** Docker, Python (FastAPI), Uvicorn, SQLite.

---

### Task 1: Create Repository Core Skeleton & Docker Composition

**Files:**
- Create: `docker-compose.yml`
- Create: `Makefile`
- Modify: `README.md:1-5`

**Step 1: Write the failing test**

```bash
docker-compose config
```

**Step 2: Run test to verify it fails**

Run: `docker-compose config`
Expected: FAIL indicating no such file.

**Step 3: Write minimal implementation**

Create standard `docker-compose.yml` declaring network and the three core services (`sdr-dsp`, `api-broker`, `pwa-frontend`), wired to a shared `navtex_net` and declaring a `sqlite_data` persistent volume. Create a simple `Makefile` for developer ergonomics.

**Step 4: Run test to verify it passes**

Run: `docker-compose config`
Expected: PASS with valid container output structure.

**Step 5: Commit**

```bash
git add docker-compose.yml Makefile README.md
git commit -m "chore: scaffold base docker-compose architecture for navtex appliance"
```

### Task 2: Scaffold the API Broker

**Files:**
- Create: `api/Dockerfile`
- Create: `api/requirements.txt`
- Create: `api/tests/test_health.py`
- Create: `api/src/main.py`

**Step 1: Write the failing test**

```python
# api/tests/test_health.py
from fastapi.testclient import TestClient
from src.main import app

client = TestClient(app)

def test_health_check():
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}
```

**Step 2: Run test to verify it fails**

Run: `pytest api/tests/test_health.py`
Expected: FAIL with "ModuleNotFoundError: No module named 'src'"

**Step 3: Write minimal implementation**

Write `api/src/main.py` configuring a barebones FastAPI app with a `/health` endpoint. Create the `api/Dockerfile` using `python:3.11-slim` and populate `api/requirements.txt`.

**Step 4: Run test to verify it passes**

Run: `pytest api/tests/`
Expected: PASS

**Step 5: Commit**

```bash
git add api/
git commit -m "feat: scaffold fastapi broker with health endpoint"
```

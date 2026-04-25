# Navtex Frontend PWA Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Build an ultra-modern, premium PWA dashboard for the Navtex receiver, featuring real-time message updates, station/message filtering, and SDR configuration.

**Architecture:** A Vite-based React application served via Nginx. Integrates with existing FastAPI backend for history (REST) and live updates (WebSockets).

**Tech Stack:** React (Vite), Vanilla CSS (Modern Aesthetics), Web Push API, Service Workers.

---

### Task 1: Scaffold Frontend & Docker integration

**Files:**
- Create: `frontend/` (Directory)
- Create: `frontend/Dockerfile`
- Modify: `docker-compose.yml`

**Step 1: Create Vite project**
Run: `npx create-vite@latest frontend --template react`

**Step 2: Add Dockerfile**
Create `frontend/Dockerfile` using `nginx:alpine` to serve `dist/`.

**Step 3: Update docker-compose.yml**
Switch `pwa-frontend` to use `build: ./frontend`.

**Step 4: Commit**
```bash
git add frontend/ docker-compose.yml
git commit -m "chore: scaffold vite/react frontend and configure docker build"
```

---

### Task 2: Implement Real-time Dashboard

**Files:**
- Create: `frontend/src/components/Dashboard.jsx`
- Create: `frontend/src/api/websocket.js`
- Modify: `frontend/src/App.css`

**Step 1: Dashboard Foundation**
Implement a glassmorphism shell with a header and main message list.

**Step 2: WebSocket Integration**
Connect to `ws://localhost:8000/ws` and update state when new messages arrive.

**Step 3: History Loading**
Fetch existing messages from `GET /messages` on mount.

**Step 4: Commit**
```bash
git add frontend/src/
git commit -m "feat(ui): implement real-time message dashboard"
```

---

### Task 3: Filtering and Notification Rules

**Files:**
- Create: `frontend/src/hooks/useFilters.js`
- Create: `frontend/src/components/FilterBar.jsx`

**Step 1: LocalStorage State**
Store B1 (Station) and B2 (Message Type) filters in local storage.

**Step 2: Filter Logic**
Implement logic to hide/show messages based on user preferences.

**Step 3: Commit**
```bash
git add frontend/src/
git commit -m "feat(ui): add B1/B2 filtering rules"
```

---

### Task 4: SDR Configuration UI & Backend Bridge

**Files:**
- Modify: `api/src/main.py`
- Create: `frontend/src/components/Settings.jsx`

**Step 1: Add config endpoint**
Implement `GET/POST /config` in FastAPI to manage antenna and gain.

**Step 2: Settings UI**
Add a tab in the PWA to adjust SDR settings.

**Step 3: Commit**
```bash
git add api/ frontend/
git commit -m "feat: implement SDR configuration settings and API"
```

---

### Task 5: PWA manifest and Service Worker

**Files:**
- Create: `frontend/public/manifest.json`
- Create: `frontend/src/sw.js`

**Step 1: Manifest**
Icons, colors, and standalone mode configuration.

**Step 2: Service Worker Service**
Register service worker for offline shell and background sync.

**Step 3: Commit**
```bash
git add frontend/
git commit -m "feat(pwa): enable manifest and service worker"
```

---

### Task 6: Web Push Notifications

**Files:**
- Modify: `frontend/src/sw.js`
- Create: `frontend/src/api/push.js`

**Step 1: Subscription Flow**
Request permission and subscribe to Push notifications.

**Step 2: Background Alerts**
Show toast alerts when filtered messages arrive while the app is backgrounded.

**Step 3: Commit**
```bash
git add frontend/
git commit -m "feat(pwa): implement native web push notifications"
```

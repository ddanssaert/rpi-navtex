# Phase 999.2: Send Push Notifications on New Messages - Context

**Gathered:** 2026-04-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement the Web Push API (RFC 8030) end-to-end: the api-broker generates VAPID keys, stores push subscriptions, and sends encrypted Web Push notifications when new Navtex messages arrive. The PWA frontend registers a push subscription and writes filter preferences to IndexedDB so the service worker can filter incoming pushes client-side.

This phase does NOT include:
- Notification click → open app navigation (Phase 999.3)
- Overhauling the filter UI from B1/B2 to human-readable labels (Phase 999.4)

</domain>

<decisions>
## Implementation Decisions

### Push Delivery Strategy
- **D-01:** Server always sends Web Push AND WebSocket broadcast simultaneously. No per-device connection tracking. The in-app suppression logic in Dashboard.jsx (already implemented) handles the duplicate — it already suppresses the toast when the user is on the dashboard tab.
- **D-02:** Service worker suppresses the OS toast notification if the PWA is open and focused. Uses `clients.matchAll()` to check for a visible focused client before calling `showNotification()`. The WebSocket message update already happened — no need to also show a toast.

### Filter Scope & Location
- **D-03:** Client-side filtering only. The server sends Web Push to **all** subscribers for every new message. The service worker reads filter preferences and silently drops pushes that don't match before calling `showNotification()`.
- **D-04:** Filter preferences are stored in **IndexedDB** (key: `navtex-filters`). The main React app writes to IndexedDB whenever the user changes their notification filter settings. The service worker reads from IndexedDB on each `push` event.

### Push Payload Content
- **D-05:** Rich payload: `{ station_id, message_type, content_snippet }` where `content_snippet` is the first 100 characters of the message content. Payload is encrypted end-to-end per the Web Push spec — push relay services (Google, Mozilla, Apple) cannot read it. The service worker formats the notification using the existing pattern: `station_id + message_type + ': ' + title`.

### VAPID Key Management
- **D-06:** VAPID keys are auto-generated on first API startup and written to `/data/vapid_keys.json` (same persistent Docker volume as the SQLite database). On subsequent startups the file is read. If the file is missing (fresh volume), new keys are generated. This is zero-config and survives container restarts and `docker-compose pull` updates.
- **D-07:** VAPID contact identifier is `mailto:` a configurable email address. Stored as `VAPID_CONTACT` environment variable in docker-compose.yml (with a sensible default like `mailto:admin@navtex.local`). Push services use this to reach the operator if there are delivery issues.

### Claude's Discretion
- Python push library selection (recommend `pywebpush` — the standard, well-maintained option)
- SQLite subscriptions table schema (endpoint, p256dh, auth, created_at — standard fields)
- API endpoint naming for subscription registration/unregistration
- Error handling for expired subscriptions (HTTP 410 from push service → delete from DB)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Existing Service Worker (critical — has push handler already)
- `frontend/public/sw.js` — Push event handler and notificationclick handler already implemented. Planner should extend this, not replace it.

### Existing Notification UI
- `frontend/src/components/Settings.jsx` — Has Notification.requestPermission() and test notification UI. Add pushManager.subscribe() here.
- `frontend/src/components/Dashboard.jsx` — Has WebSocket notification logic. The suppression behavior (lines 26-69) must be preserved.

### Existing API (extend, don't replace)
- `api/src/main.py` — POST /messages is where Web Push must be triggered (after saving to DB and WebSocket broadcast)
- `api/src/security.py` — HTTPS cert generation reference (Web Push requires secure context)

### Web Standards (research agents should consult)
- Web Push Protocol: RFC 8030
- Message Encryption: RFC 8291 (pywebpush handles this)
- VAPID: RFC 8292

### No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `frontend/public/sw.js` push event handler (lines 28-43): Already formats and shows notifications from push data. Needs extension to: (a) check `clients.matchAll()` before showing, (b) read IndexedDB for filter preferences.
- `frontend/public/sw.js` notificationclick handler (lines 45-50): Already opens app root URL. Phase 999.3 will extend this to navigate to the specific message.
- `api/src/database.py` + `api/src/models.py`: SQLite async setup to follow for new `push_subscriptions` table.
- `api/src/main.py` POST /messages: Trigger point for Web Push after DB write and WebSocket broadcast.
- `frontend/src/components/Settings.jsx` Notification.requestPermission(): Natural place to add pushManager.subscribe().

### Established Patterns
- FastAPI with aiosqlite (async) — new push_subscriptions table follows same pattern as messages table
- Pydantic schemas for request/response validation — add PushSubscription schema
- Docker volume at `/data/` — VAPID keys go here alongside navtex.db

### Integration Points
- `api/src/main.py` POST /messages: Add Web Push send after existing broadcast call
- New API routes needed: POST /push/subscribe, DELETE /push/unsubscribe, GET /push/vapid-key (returns public key for frontend)
- `frontend/src/components/Settings.jsx`: Add subscription registration flow after permission grant
- `frontend/public/sw.js`: Extend push handler with focus-check and IndexedDB filter read

</code_context>

<specifics>
## Specific Ideas

- When the PWA opens (from notification click OR cold launch), it should fetch the latest messages from the server to ensure the list is up to date. This was mentioned as a side note during delivery strategy discussion — relevant context for Phase 999.3 implementation.
- The service worker's existing notification format (`station_id + message_type + ': ' + title`) should be preserved and used for the push notification display.

</specifics>

<deferred>
## Deferred Ideas

- Notification click → navigate to specific message (Phase 999.3)
- Human-readable station/message type labels in the filter UI (Phase 999.4)
- Per-device subscription preferences stored server-side (could be revisited when Phase 999.4 redesigns filter UX)

</deferred>

---

*Phase: 999.2-send-push-notifications-on-new-messages*
*Context gathered: 2026-04-26*

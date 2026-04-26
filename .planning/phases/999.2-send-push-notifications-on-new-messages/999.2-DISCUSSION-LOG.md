# Phase 999.2: Send Push Notifications on New Messages - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-26
**Phase:** 999.2-send-push-notifications-on-new-messages
**Areas discussed:** Push delivery strategy, Filter scope & location, Push payload content, VAPID key management

---

## Push Delivery Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Always push + WebSocket | Server always sends both. In-app suppression handles dedup. | ✓ |
| Push only when offline | Track WebSocket connections per device, push only to offline ones. | |
| Push only, drop WebSocket notifications | Remove WebSocket showNotification(), rely solely on Web Push. | |

**User's choice:** Always push + WebSocket (Recommended)
**Notes:** Simplest server logic — no connection tracking needed. The existing Dashboard.jsx suppression (tab focus check) already handles the duplicate.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Suppress toast when app focused | Use clients.matchAll() — skip showNotification() if user is on dashboard | ✓ |
| Show it anyway | Always show OS toast, even if app is focused | |
| You decide | Standard PWA behavior | |

**User's choice:** Suppress the toast (Recommended)
**Notes:** Side note from user: "when the app's opened, it should also fetch the latest messages from the server" — captured as context for Phase 999.3.

---

## Filter Scope & Location

| Option | Description | Selected |
|--------|-------------|----------|
| Server-side per subscription | Each subscription stores filter prefs, server only pushes matching messages. | |
| Client-side in service worker | Push all, service worker reads IndexedDB and filters before showNotification(). | ✓ |
| No filtering yet | Send all to all, add filtering in Phase 999.4. | |

**User's choice:** Client-side in service worker
**Notes:** Simpler server logic. Phase 999.4 will overhaul the filter UI — this decision keeps 999.2 contained.

---

| Option | Description | Selected |
|--------|-------------|----------|
| IndexedDB | Service-worker accessible, offline-capable storage. | ✓ |
| postMessage from main thread | Send filter state on startup — lost if SW is terminated. | |
| You decide | Let implementation choose. | |

**User's choice:** IndexedDB (Recommended)
**Notes:** Only storage mechanism accessible from service workers without postMessage.

---

## Push Payload Content

| Option | Description | Selected |
|--------|-------------|----------|
| Rich: station, type, content snippet | station_id, message_type, first 100 chars. Encrypted. | ✓ |
| Minimal: badge only | Just {type: 'navtex_message'}, no content details. | |
| Full message | Entire message record — may exceed 4096 byte limit. | |

**User's choice:** Rich: station, type, content snippet (Recommended)
**Notes:** Web Push spec encrypts payloads end-to-end. Consistent with existing sw.js notification format.

---

## VAPID Key Management

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-generated, persisted in /data/ | Generated on first startup, saved to /data/vapid_keys.json. | ✓ |
| Env vars in docker-compose.yml | Pre-generate and paste into env — manual setup step. | |
| Regenerated on startup | Fresh keys every restart — breaks all subscriptions. | |

**User's choice:** Auto-generated, persisted in /data/ volume (Recommended)
**Notes:** Zero-config. Matches the philosophy of the appliance design. Volume wipe = re-subscribe (acceptable).

---

| Option | Description | Selected |
|--------|-------------|----------|
| mailto: configurable email | VAPID_CONTACT env var in docker-compose, default mailto:admin@navtex.local | ✓ |
| https: device IP | Local IPs change, may confuse push services. | |
| You decide | Hardcoded default. | |

**User's choice:** mailto: a configurable email (Recommended)

---

## Claude's Discretion

- Python push library selection
- SQLite subscriptions table schema
- API endpoint naming for subscribe/unsubscribe
- Error handling for expired/invalid subscriptions (HTTP 410)

## Deferred Ideas

- Notification click → open app and navigate to specific message (Phase 999.3)
- Human-readable filter labels (Phase 999.4)
- Server-side per-subscription filter preferences (could revisit with Phase 999.4)

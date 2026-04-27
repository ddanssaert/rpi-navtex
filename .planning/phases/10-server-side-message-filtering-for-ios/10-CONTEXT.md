# Phase 10: Server-Side Message Filtering for iOS - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Move push notification filtering from client-side (never fully implemented — sw.js currently shows all notifications) to server-side, so the backend only sends pushes that match each subscriber's preferences. This enables iOS support since iOS blocks client-side filtering in service workers.

Scope:
- Add filter preferences to the server-side subscription model
- Filter push sends in the backend before dispatching to each subscriber
- Sync filter preferences from the React app to the server when the user changes their filters
- Remove any client-side push filtering code in sw.js (Phase 9 already left sw.js unconditional)

Not in scope:
- In-app Dashboard display filtering (stays in localStorage via useFilters.js — unchanged)
- Human-readable filter labels (Phase 4)
- VAPID or subscription registration changes (Phase 9)
- Notification deep-linking (Phase 3 — done)

</domain>

<decisions>
## Implementation Decisions

### Phase 4 dependency
- **D-01:** Proceed with the current filter schema `{ stations: [], types: [] }` — station IDs (single chars) and message types (single chars). Phase 4 changes labels in the UI, not the underlying data model. Phase 10 plans are not blocked by Phase 4 being unplanned.

### Cross-platform scope
- **D-02:** Server-side filtering replaces client-side filtering for **all platforms** — Android, iOS, desktop. No platform detection, no dual code path. One server-side filter system for all push subscribers.
- **D-03:** In-app Dashboard display filtering (what messages the user sees in the React app) stays in **localStorage** via `useFilters.js`. Server-side filtering is exclusively for push notification delivery. The two systems are independent.

### Claude's Discretion
- Filter storage schema: JSON column on `push_subscriptions` vs. a separate `subscription_filters` table (recommend JSON column — simpler, no join needed for push send)
- Filter sync strategy: when filters change in the UI, when do they reach the server? Options: immediate API call on every filter change (debounced), or only synced when subscription is registered/refreshed (recommend immediate debounced call — keeps push delivery preferences up to date)
- Default behavior when no filters are stored for a subscription: send all messages (matches current `useFilters.js` empty-array = ALL semantic)
- New API endpoint design for updating subscription filters (e.g., PUT /push/subscription/filters or PATCH /push/subscribe)
- Whether to reuse the existing `db.js` IndexedDB utilities, or remove them now that push filtering moves server-side

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Backend (primary change targets)
- `api/src/models.py` — `PushSubscription` model. Needs a `filters` column (JSON). Current columns: id, endpoint, p256dh, auth, created_at.
- `api/src/push_utils.py` — `send_push_notification()`. Filter logic must be applied before calling this per-subscriber. Or a new helper that takes filter preferences.
- `api/src/main.py` — POST /messages: the push-send loop that iterates all subscriptions. Filter evaluation goes here.

### Frontend (filter state + sync)
- `frontend/src/hooks/useFilters.js` — Current client-side filter state in localStorage. This is the source of truth to sync to the server when filters change. Logic: empty array = ALL.
- `frontend/src/components/FilterBar.jsx` — Filter UI where users change preferences. Where the server sync call should be triggered.
- `frontend/src/components/Settings.jsx` — Subscription management. May need to send current filters when registering a new subscription.
- `frontend/src/utils/db.js` — IndexedDB utilities. Unused by push filtering (useFilters.js uses localStorage). Evaluate whether to keep or remove.

### Prior phase decisions being superseded/extended
- `.planning/phases/02-send-push-notifications-on-new-messages/02-CONTEXT.md` — D-03/D-04 specified IndexedDB client-side filtering. Phase 10 supersedes this: server-side replaces it.
- `.planning/phases/09-ios-background-push-notifications/09-CONTEXT.md` — D-07: Phase 9 explicitly left sw.js unconditional (no filtering). Phase 10 builds on this — filtering is server-side, sw.js stays unconditional.

### Database migration
- `api/alembic/versions/002_add_push_subscriptions.py` — Existing migration for push_subscriptions table. New migration needed to add `filters` column.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `useFilters.js`: returns `{ filters, toggleStation, toggleType, isFiltered }`. The `filters` object (`{ stations: [], types: [] }`) is exactly what needs to be synced to the server. No changes to useFilters.js needed — just add a side-effect that POSTs/PATCHes to the server when `filters` changes.
- `api/src/push_utils.py` `send_push_notification()`: already handles WebPushException and 410. Filter check wraps the loop that calls this.
- `api/alembic/` migrations: pattern to follow for adding `filters` column to `push_subscriptions`.

### Established Patterns
- FastAPI with SQLAlchemy async — new filter column follows same pattern as existing models
- `useFilters.js` useEffect for localStorage — add a parallel useEffect for server sync
- Empty filter arrays = "include all" (both `useFilters.js` `isFiltered()` and the new server-side logic must share this semantic)

### Integration Points
- POST /messages (main.py): the push loop — filter each subscription's stored preferences against `message.station_id` and `message.message_type` before sending
- Filter change in `FilterBar.jsx` → `toggleStation`/`toggleType` in `useFilters.js` → new API call to update server-side filters for the active subscription
- New subscription registration (Settings.jsx) → must include current filters in the registration payload so the server has preferences from day one

</code_context>

<specifics>
## Specific Ideas

- The filter sync on change should use the active push subscription's endpoint as the identifier (not a user ID — this app has no auth). The API can look up the subscription by endpoint and update its filters.
- The `filters` column could default to `NULL` in the DB, with `NULL` meaning "no preference = send all" — avoids a migration step to backfill existing subscriptions.

</specifics>

<deferred>
## Deferred Ideas

- Per-device filter preferences stored server-side with user accounts (no auth in this app — subscriptions are the identity)
- Filter analytics / delivery stats per subscriber
- Human-readable filter labels in the API response (Phase 4)

</deferred>

---

*Phase: 10-server-side-message-filtering-for-ios*
*Context gathered: 2026-04-27*

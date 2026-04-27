# Phase 9: iOS Background Push Notifications - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Fix known iOS Web Push delivery issues so the PWA receives background push notifications on iOS 16.4+. Android already works. This phase targets delivery reliability — it does NOT implement client-side filtering (deferred to Phase 10 server-side approach).

Scope:
- Strip iOS-incompatible service worker notification options
- Add focus-check before showNotification (Phase 2 D-02, finally implementing)
- Add re-subscription check on app open (iOS subscriptions expire silently)
- Backend: let pywebpush handle APNs transparently, delete on HTTP 410

Not in scope:
- IndexedDB client-side filtering (Phase 2 D-03/D-04 — Phase 10 will replace with server-side)
- Server-side filtering refactor (Phase 10)
- Notification deep-link navigation (Phase 3 — already done)

</domain>

<decisions>
## Implementation Decisions

### Diagnosis approach
- **D-01:** Skip investigation phase — root causes are known. Plan goes straight to fixes. Physical iOS device is available for end-to-end verification.

### APNs / backend handling
- **D-02:** Let `pywebpush` handle APNs (web.push.apple.com) transparently — no iOS-specific branch needed in `main.py`. pywebpush 2.x speaks standard Web Push protocol for all push services including APNs.
- **D-03:** HTTP 410 response (expired/invalid subscription) → delete from DB, same logic as Android. iOS subscriptions expire more frequently but no special handling needed.

### Service worker fixes
- **D-04:** Remove `renotify: true` from the notification options in `sw.js`. This option can cause iOS to silently suppress notifications. Safe to remove — Android does not depend on it for correct behavior.
- **D-05:** Remove `badge: '/favicon.svg'` from notification options in `sw.js`. Not supported on iOS WebKit and can trigger silent failures.
- **D-06:** Add `clients.matchAll({ includeUncontrolled: true, type: 'window' })` focus check before `self.registration.showNotification()`. If the PWA is open and focused, skip the OS toast. This implements Phase 2 D-02 which was never built. On iOS background, no client is focused, so `showNotification()` always fires as expected.

### Filtering boundary
- **D-07:** Phase 9 does NOT implement IndexedDB client-side filtering (Phase 2 D-03/D-04). The `sw.js` push handler shows all notifications unconditionally. Phase 10 will add server-side per-subscriber filtering instead.

### Re-subscription on app open
- **D-08:** On PWA app open, check if the current push subscription is still valid (call `pushManager.getSubscription()`, verify the endpoint is not expired). If subscription is missing or invalid, silently re-subscribe and POST the new subscription to the API. This catches the iOS silent-expiry problem. Location: `frontend/src/components/Settings.jsx` or a dedicated hook called from the app root.

### Claude's Discretion
- Where exactly in the React component tree to place the re-subscription check (hook vs. Settings.jsx vs. App.jsx effect)
- Whether to use a `useEffect` in the root component or an explicit re-subscription button in Settings.jsx
- Error logging format for failed push sends in `main.py`

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Service worker (primary change target)
- `frontend/public/sw.js` — Push event handler to modify. Remove `renotify`/`badge`, add focus-check. Do NOT replace, only edit.

### Frontend subscription management
- `frontend/src/components/Settings.jsx` — Existing `Notification.requestPermission()` and subscription UI. Re-subscription check likely goes here or nearby.
- `frontend/src/components/Dashboard.jsx` — Existing WebSocket notification suppression logic (lines 26-69). Must be preserved — it complements the focus-check being added to sw.js.

### Phase 2 decisions (context for what was and wasn't built)
- `.planning/phases/02-send-push-notifications-on-new-messages/02-CONTEXT.md` — D-02 (focus-check), D-03/D-04 (IndexedDB filter). Phase 9 implements D-02. D-03/D-04 are explicitly NOT implemented here (Phase 10).

### Backend push sending
- `api/src/main.py` — POST /messages is where Web Push is triggered. 410 handling should be confirmed here.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `frontend/public/sw.js` push handler (lines 51-85): Already formats and shows notifications. Phase 9 edits this in place — remove `renotify`/`badge` from the options object, wrap `showNotification()` in a `clients.matchAll()` guard.
- `frontend/src/components/Settings.jsx`: Natural location for the re-subscription check — already has `pushManager.subscribe()` and Notification permission logic.

### Established Patterns
- Service worker uses `event.waitUntil(async IIFE)` pattern — focus-check goes inside this IIFE before the `showNotification()` call.
- Backend uses HTTP 410 → delete subscription (standard Web Push expiry handling).

### Integration Points
- `sw.js` push event handler: wrap `showNotification()` with focus check
- `Settings.jsx` or app root: add `pushManager.getSubscription()` check on mount and re-register if missing

</code_context>

<specifics>
## Specific Ideas

- iOS requires the PWA to be added to the home screen to receive push. This is a prerequisite the user must meet — not something code can fix. Worth noting in the verification checklist.
- The focus-check pattern: `const clients = await self.clients.matchAll({ includeUncontrolled: true, type: 'window' }); const focused = clients.some(c => c.visibilityState === 'visible'); if (!focused) { await self.registration.showNotification(title, options); }`

</specifics>

<deferred>
## Deferred Ideas

- IndexedDB client-side filtering from Phase 2 D-03/D-04 — explicitly not implemented here. Phase 10 replaces with server-side approach.
- Separate logging/tracking of iOS subscription churn vs. Android (could be useful diagnostics, deferred as low-priority).

</deferred>

---

*Phase: 09-ios-background-push-notifications*
*Context gathered: 2026-04-27*

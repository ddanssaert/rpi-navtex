# Phase 10: Server-Side Message Filtering for iOS - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 10-server-side-message-filtering-for-ios
**Areas discussed:** Phase 4 dependency handling, Cross-platform scope

---

## Phase 4 dependency handling

| Option | Description | Selected |
|--------|-------------|----------|
| Proceed with current schema | Filter data model {stations:[], types:[]} is stable — Phase 4 changes labels only | ✓ |
| Wait for Phase 4 first | Safer if Phase 4 might change the filter data model substantially | |

**User's choice:** Proceed with current filter schema

---

## Cross-platform scope

| Option | Description | Selected |
|--------|-------------|----------|
| Replace everywhere — one system | Server-side for all platforms, single code path, drop client-side push filtering | ✓ |
| iOS-only — keep client-side for others | Server-side for iOS subscriptions, client-side IndexedDB for Android/Chrome | |

**User's choice:** Replace everywhere — unified server-side system

| Option | Description | Selected |
|--------|-------------|----------|
| Keep display filters in localStorage | Server-side only for push delivery; Dashboard display stays localStorage | ✓ |
| Unify — single filter store for both | One filter state for Dashboard display AND push delivery | |

**User's choice:** Keep display filters in localStorage (separate concerns)

---

## Claude's Discretion

- Filter storage schema on push_subscriptions (JSON column vs. separate table)
- Filter sync timing (immediate debounced API call vs. on subscription registration)
- Default behavior for subscriptions with no stored filters (send all)
- API endpoint design for updating subscription filters
- Whether to remove db.js IndexedDB utilities now that push filtering moves server-side

## Deferred Ideas

- Per-device filter preferences with user accounts (no auth in this app)
- Filter delivery analytics per subscriber
- Human-readable filter labels in API responses (Phase 4)

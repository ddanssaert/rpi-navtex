# Phase 9: iOS Background Push Notifications - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 09-ios-background-push-notifications
**Areas discussed:** Diagnosis approach, APNs backend handling, Service worker iOS fixes, Boundary with Phase 10

---

## Diagnosis approach

| Option | Description | Selected |
|--------|-------------|----------|
| Known fixes only | Go straight to fixes — sw.js incompatibilities and APNs issues are well-documented | ✓ |
| Investigation first, then fix | Start with diagnostic step: inspect iOS subscription endpoints, check logs | |

**User's choice:** Known fixes only

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, physical iOS device | Can test end-to-end (required for push — simulators don't support push) | ✓ |
| Limited / no device | Testing relies on logs and endpoint inspection | |

**User's choice:** Physical iOS device available

---

## APNs backend handling

| Option | Description | Selected |
|--------|-------------|----------|
| Let pywebpush handle it transparently | pywebpush 2.x handles APNs endpoints via standard Web Push — no iOS branch needed | ✓ |
| Add iOS-specific backend branch | Detect web.push.apple.com and apply iOS-specific headers/TTL explicitly | |

**User's choice:** Let pywebpush handle it transparently

| Option | Description | Selected |
|--------|-------------|----------|
| Delete on 410, same as Android | iOS subscriptions expire more often but same 410 → delete logic applies | ✓ |
| Log expired iOS subs separately | Track iOS-specific churn for diagnostics | |

**User's choice:** Delete on 410, same as Android

---

## Service worker iOS fixes

| Option | Description | Selected |
|--------|-------------|----------|
| Remove renotify and badge globally | Strip both — safe on Android too, eliminates iOS silent-failure risk | ✓ |
| Feature-detect per platform | Keep for Android, conditionally omit for iOS via userAgent/endpoint check | |

**User's choice:** Remove globally

| Option | Description | Selected |
|--------|-------------|----------|
| Add focus-check in Phase 9 | Small change, already spec'd in Phase 2 D-02 | ✓ |
| Defer to Phase 10 | Phase 10 restructures push handling anyway | |

**User's choice:** Add focus-check in Phase 9

---

## Boundary with Phase 10

| Option | Description | Selected |
|--------|-------------|----------|
| Phase 9 skips filtering entirely | Don't implement IndexedDB filter — Phase 10 does server-side instead | ✓ |
| Phase 9 implements a no-op filter placeholder | Add IndexedDB read that always passes through — scaffolding for Phase 10 | |

**User's choice:** Skip filtering entirely

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, add re-subscription check on app open | iOS subscriptions expire silently — check on app open and re-register if needed | ✓ |
| No, out of scope | Focus on delivery fix only | |

**User's choice:** Yes, add re-subscription check on app open

---

## Claude's Discretion

- Where exactly in React component tree to place re-subscription check (hook vs. Settings.jsx vs. App.jsx)
- Whether to use a useEffect in root component or explicit re-subscription button in Settings.jsx
- Error logging format for failed push sends in main.py

## Deferred Ideas

- IndexedDB client-side filtering (Phase 2 D-03/D-04) → Phase 10 server-side
- Separate iOS subscription churn logging → low-priority diagnostic feature

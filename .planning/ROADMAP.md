# Roadmap: RPi Navtex Receiver

## Overview

A headless Raspberry Pi 3B platform leveraging the SDRPlay RSPDX for receiving and decoding Navtex on 490kHz/518kHz. The platform targets zero-configuration appliance-level reliability, delivering messages via a Progressive Web App (PWA) with push notifications.

## Phases

- [x] **Phase 1: Dynamic Settings Propagation** - UI settings for Antenna and Gain take effect immediately
- [x] **Phase 2: Send Push Notifications on New Messages** - Web Push API end-to-end with native OS toast alerts
- [x] **Phase 3: Open App When Clicking on Push Notification** - Notification click opens/focuses PWA
- [ ] **Phase 4: Human-Readable Mapping & Filters** - Replace B1/B2 codes with readable labels and batch filter actions
- [ ] **Phase 5: Configurable Message Rotation** - Retention period UI and background pruning job
- [ ] **Phase 6: Global Output Configuration** - Centralized SignalK/Garmin output settings
- [ ] **Phase 7: Mobile UI Polish** - Mobile-specific layouts using Stitch components
- [x] **Phase 8: Raspberry Pi Appliance OS Image** - Flashable RPi OS image with Docker stack, headless WiFi/SSH setup, fallback hotspot
- [ ] **Phase 9: iOS Background Push Notifications** - Investigate and fix push notifications on iOS when PWA is backgrounded
- [ ] **Phase 10: Server-Side Message Filtering for iOS** - Refactor client-side filtering to server-side to support iOS PWA constraints

## Phase Details

### Phase 1: Dynamic Settings Propagation

**Goal:** UI settings for Antenna and Gain take effect immediately without restarting the DSP pipeline.
**Depends on:** Nothing (first phase)
**Requirements:** PROP-01, PROP-02, PROP-03, PROP-04
**Plans:** 2 plans

Plans:
- [x] 01-01-PLAN.md — sdr-dsp control server + sdrplay_api_Update live dispatch
- [x] 01-02-PLAN.md — api-broker async forward, tests, docker-compose, end-to-end checkpoint

### Phase 2: Send Push Notifications on New Messages

**Goal:** api-broker sends Web Push notifications when new Navtex messages arrive; PWA frontend receives them as native toast alerts.
**Depends on:** Nothing (can run independently)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [x] TBD

### Phase 3: Open App When Clicking on Push Notification

**Goal:** Tapping a push notification opens or focuses the PWA and navigates to the relevant message.
**Depends on:** Phase 2
**Requirements:** TBD
**Plans:** TBD

Plans:
- [x] TBD

### Phase 4: Human-Readable Mapping & Filters

**Goal:** Replace B1/B2 codes with "Stations"/"Message Types" labels; add lookup table for A-Z stations and message types. Notification filters in Settings.jsx use the same lookup for consistency. (Batch select dropped per user decision.)
**Depends on:** Nothing (can run independently)
**Requirements:** D-01, D-02, D-03, D-04, D-05, D-06, D-07, D-08, D-09, D-10, D-11
**Plans:** 3 plans

Plans:
- [ ] 04-01-PLAN.md — Create navtex-codes.js lookup module + update MessageCard.jsx badges
- [ ] 04-02-PLAN.md — Rewrite FilterBar.jsx with collapsible checkbox sections
- [ ] 04-03-PLAN.md — Update Settings.jsx notification filter UI to use lookup + checkboxes

### Phase 5: Configurable Message Rotation

**Goal:** Keep SD card storage manageable with a configurable retention period (default 30 days) and a periodic background pruning job in api-broker.
**Depends on:** Nothing (can run independently)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 6: Global Output Configuration

**Goal:** Centralize third-party output settings (Garmin GPSMap, SignalK) in a server-side "Outputs" configuration section with persistent storage.
**Depends on:** Nothing (can run independently)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 7: Mobile UI Polish

**Goal:** Mobile-specific layouts for the message list and system diagnostics using Stitch components for improved responsiveness.
**Depends on:** Phase 4 (filter UI should be in its final form first)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 8: Raspberry Pi Appliance OS Image

**Goal:** Produce a flashable RPi OS Lite image with the navtex Docker stack pre-installed, booting headless with zero post-flash configuration. WiFi credentials, SSH, and hostname are injected at flash time via Raspberry Pi Imager's OS customisation flow. A NetworkManager fallback hotspot (`Navtex-AP`) activates when no known network is reachable.
**Depends on:** Nothing (can run independently)
**Requirements:** D-01, D-02, D-03, D-04, D-05, D-06
**Plans:** 3 plans

Plans:
- [x] 08-01-PLAN.md — Convert docker-compose.yml to host networking (D-05)
- [x] 08-02-PLAN.md — pi-gen custom stage: Docker, navtex stack, avahi, AccessPopup open Navtex-AP (D-01, D-02, D-06)
- [x] 08-03-PLAN.md — CI release pipeline + RPi Imager manifest (D-03, D-04)

### Phase 9: iOS Background Push Notifications

**Goal:** Fix iOS Web Push delivery when the PWA is backgrounded. Strip iOS-incompatible service worker options (`badge`, `renotify`), add a focus-check guard before `showNotification`, and silently re-subscribe on app open to recover from iOS silent subscription expiry.
**Depends on:** Phase 2
**Requirements:** D-04, D-05, D-06, D-08
**Plans:** 2 plans

Plans:
- [ ] 09-01-PLAN.md — Fix sw.js: remove `badge`/`renotify`, add focus-check guard (D-04, D-05, D-06)
- [ ] 09-02-PLAN.md — Settings.jsx: silent re-subscription on app open when subscription expired (D-08)

### Phase 10: Server-Side Message Filtering for iOS

**Goal:** Refactor message filtering from client-side to server-side so iOS (which blocks client-side filtering in PWAs) and all other platforms receive only the pushes matching each subscriber's stored preferences. In-app Dashboard filtering stays in localStorage; server-side filtering is exclusively for push delivery.
**Depends on:** Phase 9 (push subscriptions table, sw.js unconditional)
**Requirements:** FILT-01, FILT-02, FILT-03, FILT-04
**Plans:** 2 plans

Plans:
- [ ] 10-01-PLAN.md — Backend: migration 003 + filters column + _matches_filters + PUT /push/filters
- [ ] 10-02-PLAN.md — Frontend: useFilters server sync (debounced) + Dashboard prop wiring + Settings refactor

## Backlog

Backlog items live at 999.x — unsequenced ideas not yet ready for active planning.
Use `/gsd-discuss-phase <N>` to explore, `/gsd-review-backlog` to promote.

### Phase 999.5: System Diagnostics Tab (BACKLOG)

**Goal:** Dedicated "System" tab with real-time log streaming from all containers, hardware performance telemetry (signal level, CPU), and green/yellow/red status indicators per microservice.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Dynamic Settings Propagation | 2/2 | Done | 2026-04-27 |
| 2. Send Push Notifications | TBD/TBD | Done | 2026-04-27 |
| 3. Push Notification Deep Link | TBD/TBD | Done | 2026-04-27 |
| 4. Human-Readable Mapping | 0/3 | Not started | - |
| 5. Message Rotation | 0/TBD | Not started | - |
| 6. Global Output Config | 0/TBD | Not started | - |
| 7. Mobile UI Polish | 0/TBD | Not started | - |
| 8. Raspberry Pi Appliance OS Image | 3/3 | Done | 2026-04-27 |
| 999.5 System Diagnostics Tab | 0/TBD | Backlog | - |
| 9. iOS Background Push Notifications | 0/TBD | Not started | - |
| 10. Server-Side Message Filtering for iOS | 0/TBD | Not started | - |

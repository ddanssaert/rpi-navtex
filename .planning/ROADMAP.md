# Roadmap: RPi Navtex Receiver

## Overview

A headless Raspberry Pi 3B platform leveraging the SDRPlay RSPDX for receiving and decoding Navtex on 490kHz/518kHz. The platform targets zero-configuration appliance-level reliability, delivering messages via a Progressive Web App (PWA) with push notifications.

## Phases

- [ ] **Phase 1: Dynamic Settings Propagation** - UI settings for Antenna and Gain take effect immediately
- [ ] **Phase 2: Send Push Notifications on New Messages** - Web Push API end-to-end with native OS toast alerts
- [ ] **Phase 3: Open App When Clicking on Push Notification** - Notification click opens/focuses PWA
- [ ] **Phase 4: Human-Readable Mapping & Filters** - Replace B1/B2 codes with readable labels and batch filter actions
- [ ] **Phase 5: Configurable Message Rotation** - Retention period UI and background pruning job
- [ ] **Phase 6: Global Output Configuration** - Centralized SignalK/Garmin output settings
- [ ] **Phase 7: Mobile UI Polish** - Mobile-specific layouts using Stitch components

## Phase Details

### Phase 1: Dynamic Settings Propagation

**Goal:** UI settings for Antenna and Gain take effect immediately without restarting the DSP pipeline.
**Depends on:** Nothing (first phase)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 2: Send Push Notifications on New Messages

**Goal:** api-broker sends Web Push notifications when new Navtex messages arrive; PWA frontend receives them as native toast alerts.
**Depends on:** Nothing (can run independently)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 3: Open App When Clicking on Push Notification

**Goal:** Tapping a push notification opens or focuses the PWA and navigates to the relevant message.
**Depends on:** Phase 2
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

### Phase 4: Human-Readable Mapping & Filters

**Goal:** Replace B1/B2 codes with "Stations"/"Message Types" labels; add lookup table for A-Z stations and message types; add batch select actions and separate display vs. notification filters.
**Depends on:** Nothing (can run independently)
**Requirements:** TBD
**Plans:** TBD

Plans:
- [ ] TBD

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
| 1. Dynamic Settings Propagation | 0/TBD | Not started | - |
| 2. Send Push Notifications | 0/TBD | Not started | - |
| 3. Push Notification Deep Link | 0/TBD | Not started | - |
| 4. Human-Readable Mapping | 0/TBD | Not started | - |
| 5. Message Rotation | 0/TBD | Not started | - |
| 6. Global Output Config | 0/TBD | Not started | - |
| 7. Mobile UI Polish | 0/TBD | Not started | - |
| 999.5 System Diagnostics Tab | 0/TBD | Backlog | - |

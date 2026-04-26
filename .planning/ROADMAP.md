# Roadmap: RPi Navtex Receiver

## Overview

A headless Raspberry Pi 3B platform leveraging the SDRPlay RSPDX for receiving and decoding Navtex on 490kHz/518kHz. The platform targets zero-configuration appliance-level reliability, delivering messages via a Progressive Web App (PWA) with push notifications.

## Phases

No active phases planned yet. See Backlog below.

## Backlog

Backlog items live at 999.x — unsequenced ideas not yet ready for active planning.
Use `/gsd-discuss-phase <N>` to explore, `/gsd-review-backlog` to promote.

### Phase 999.1: Dynamic Settings Propagation (BACKLOG)

**Goal:** UI settings for Antenna and Gain take effect immediately without restarting the DSP pipeline.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.2: Send Push Notifications on New Messages (BACKLOG)

**Goal:** api-broker sends Web Push notifications when new Navtex messages arrive; PWA frontend receives them as native toast alerts.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.3: Open App When Clicking on Push Notification (BACKLOG)

**Goal:** Tapping a push notification opens or focuses the PWA and navigates to the relevant message.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.4: Human-Readable Mapping & Filters (BACKLOG)

**Goal:** Replace B1/B2 codes with "Stations"/"Message Types" labels; add lookup table for A-Z stations and message types; add batch select actions and separate display vs. notification filters.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.5: System Diagnostics Tab (BACKLOG)

**Goal:** Dedicated "System" tab with real-time log streaming from all containers, hardware performance telemetry (signal level, CPU), and green/yellow/red status indicators per microservice.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.6: Configurable Message Rotation (BACKLOG)

**Goal:** Keep SD card storage manageable with a configurable retention period (default 30 days) and a periodic background pruning job in api-broker.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.7: Global Output Configuration (BACKLOG)

**Goal:** Centralize third-party output settings (Garmin GPSMap, SignalK) in a server-side "Outputs" configuration section with persistent storage.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.8: Mobile UI Polish (BACKLOG)

**Goal:** Mobile-specific layouts for the message list and system diagnostics using Stitch components for improved responsiveness.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 999.1 Dynamic Settings Propagation | 0/TBD | Backlog | - |
| 999.2 Push Notifications | 0/TBD | Backlog | - |
| 999.3 Push Notification Deep Link | 0/TBD | Backlog | - |
| 999.4 Human-Readable Mapping | 0/TBD | Backlog | - |
| 999.5 System Diagnostics Tab | 0/TBD | Backlog | - |
| 999.6 Message Rotation | 0/TBD | Backlog | - |
| 999.7 Global Output Config | 0/TBD | Backlog | - |
| 999.8 Mobile UI Polish | 0/TBD | Backlog | - |

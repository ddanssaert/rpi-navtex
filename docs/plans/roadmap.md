# Navtex Project Roadmap & Backlog

This document outlines the next phases of development for the RPi Navtex Receiver, focusing on reliability, user experience, and long-term maintenance.

## Phase 1: Reliability & Signal Debugging

### [BACKLOG] [FEAT] Dynamic Settings Propagation
- **Requirement:** UI settings for Antenna and Gain must take effect immediately.
- **Implementation:**
    - `sdr-dsp` will monitor `/data/config.json` for changes.
    - Upon change, re-run `sdrplay_api_Uninit` and `sdrplay_api_Init` with new parameters.
    - Log confirmation of new hardware state.

---

## Phase 2: UI/UX & Information Architecture

### [BACKLOG] [FEAT] Send push notifications on new messages
- **Requirement:** Send push notifications on new messages.
- **Implementation:**
    - `api-broker` will send push notifications on new messages.
    - `pwa-frontend` will receive push notifications on new messages.

### [BACKLOG] [FEAT] Open app when clicking on push notification
- **Requirement:** Open app when clicking on push notification.
- **Implementation:**
    - `pwa-frontend` will open app when clicking on push notification.

### [BACKLOG] [FEAT] Human-Readable Mapping & Filters
- **No More Jargon:** Replace "B1" and "B2" with "Stations" and "Message Types" across the UI.
- **Lookup Table:**
    - Implement a global mapping for A-Z stations and message types.
    - Display names (e.g., "Navigational Warning") instead of just codes.
- **Batch Actions:** Add "Select All" / "Deselect All" to filter headers.
- **Filter Isolation:** Separate "Display Filters" (Dashboard) from "Notification Filters" (Settings/Client-side).

### [BACKLOG] [FEAT] System Diagnostics Tab
- **Logs:** Real-time stream of `sdr-dsp` and `api-broker` logs.
- **Metrics:**
    - **Signal:** Avg signal level, noise floor estimation.
    - **Hardware:** Uptime, current antenna/gain, CPU usage.
    - **Database:** Message count, storage used.
- **Status Indicators:** Green/Yellow/Red status for individual microservices.

---

## Phase 3: Infrastructure & Extensibility

### [BACKLOG] [FEAT] Configurable Message Rotation
- **Requirement:** Keep the database size manageable.
- **Implementation:**
    - Retention period configurable via UI (Default: 30 days).
    - Periodic background job in `api-broker` to prune old records.

### [BACKLOG] [FEAT] Global Output Configuration
- **Objective:** Centralize settings for third-party clients (Garmin Gpsmap, SignalK).
- **Implementation:** 
    - Add an "Outputs" configuration section in the API.
    - Ensure these settings are persisted globally on the server.

---

## Phase 4: Mobile & Ecosystem

### [BACKLOG] [FEAT] Mobile UI Polish
- **Technology:** Leverage Stitch components for better mobile responsiveness.
- **UX:** Mobile-specific layout for the message list and system diagnostics.

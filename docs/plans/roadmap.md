# Navtex Project Roadmap & Backlog

This document outlines the next phases of development for the RPi Navtex Receiver, focusing on reliability, user experience, and long-term maintenance.

## Phase 1: Reliability & Signal Debugging

### [BACKLOG] [BUG] Thorough Reception Investigation
- **Problem:** Software is silent despite messages being sent.
- **Approach:** 
    - Implement `verbose` logging in binary DSP components (`decoder`, `fsk_demod`).
    - Add logic to track FSK synchronization attempts in logs.
- **Testing:** 
    - Implement an **IQ Replay Mode** in `sdr-dsp` that reads raw `xi`/`xq` samples from a file instead of the SDRPlay API.
    - Create/obtain test IQ files for 490kHz/518kHz Navtex signals.

### [BACKLOG] [FEAT] Dynamic Settings Propagation
- **Requirement:** UI settings for Antenna and Gain must take effect immediately.
- **Implementation:**
    - `sdr-dsp` will monitor `/data/config.json` for changes.
    - Upon change, re-run `sdrplay_api_Uninit` and `sdrplay_api_Init` with new parameters.
    - Log confirmation of new hardware state.

---

## Phase 2: UI/UX & Information Architecture

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

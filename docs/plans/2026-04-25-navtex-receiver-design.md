# Navtex Receiver Platform Design

## Background & Objectives
A highly robust, headless distribution image for the Raspberry Pi 3B designed to leverage the SDRPlay RSPDX for receiving and decoding Navtex on 490kHz/518kHz. The platform aims for zero-configuration setup, appliance-level reliability, and messaging delivered via a modern Progressive Web App (PWA). 

## 1. Operating System & Delivery Architecture
- **Base OS:** Lightweight Raspberry Pi OS Lite or a ruggedized container-optimzed host.
- **Networking Daemon:** `NetworkManager` attempts to join known vessel networks. Upon failure, it seamlessly falls back to broadcasting its own Access Point (`Navtex-AP`), guaranteeing zero-lockout.
- **Containerized Stack:** The application pipeline is segmented into Docker containers via `docker-compose`, isolating faults perfectly.
- **Web Updater UI:** An integrated updater API interacts with the host Docker daemon. Over-The-Air software updates (`docker pull` and container restart) are triggered directly from the PWA Settings tab, entirely avoiding CLI interactions.

## 2. SDR Interface & Backend Data Flow
- **Hardware Configuration:** The SDRPlay RSPDX USB device is passed directly to the `sdr-dsp` container. Hardware tuning parameters (e.g., Antenna selection, LNA Gain, AGC settings) are configurable via endpoints routed from the UI settings tab.
- **Demodulation Pipeline:** Built on `bartelvdh`'s C++ DSP core, it applies bespoke FIR filters and parses the continuous RF sequence into structured Navtex object blocks.
- **Broker & Persistence:** The decoded blocks bypass static interval polling and feed directly into a modern backend broker (Node.js/Python). The broker instantly:
  - Writes to SQLite (mounted via a persistent Docker Volume).
  - Broadcasts precisely to all active web clients over WebSockets.

## 3. Frontend PWA & Client Experience
- **Progressive Delivery:** Delivered exclusively as a PWA (bypassing all proprietary App Stores).
- **Modern Aesthetics:** The user interface will be meticulously designed utilizing Google Stitch to provide an ultra-modern, responsive sailing dashboard.
- **Smart Push Notifications:** Employs the native Web Push API (iOS 16.4+ / Android) for true background Toast alerts. Users maintain strict control via **B1 (Station) and B2 (Message Type) Notification Filter rules**, ensuring they are only alerted to topics they care about.
- **Settings Dashboard:** Comprehensive management including SDR antenna adjustments and one-click Docker software updates.

## 4. Future Extensibility
- **SignalK Subsystem:** A specialized architectural module that pushes Navtex data conforming to the SignalK / NMEA 2000 standard directly to Garmin GPSMap or Victron Cerbo GX headunits.
- **MQTT Integration:** Standard IoT integration for smart-boat automations.

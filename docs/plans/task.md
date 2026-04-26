# SDR-DSP Pipeline - Task Tracker

| Task | Status | Notes |
|------|--------|-------|
| Task 1: Scaffold sdr-dsp container directory and CMake build | done | Committed: `chore: scaffold sdr-dsp container with cmake build system` |
| Task 2: Port FIR filter chain (fir1, fir2) | done | Committed: `feat(sdr-dsp): port FIR filter chain as injectable callback classes` |
| Task 3: Port FSK Decoder | done | Committed: `feat(sdr-dsp): port FSK decoder with injected bit callback` |
| Task 4: Port SITOR-B Byte State Machine (nav_b_sm) | done | Committed: `feat(sdr-dsp): port SITOR-B state machine with injected message callback` |
| Task 5: Implement Message Publisher (HTTP POST to api-broker) | done | Committed: `feat(sdr-dsp): implement HTTP POST message publisher to api-broker` |
| Task 6: Wire full pipeline in main.cpp with condition-variable IQ queue | done | Committed: `feat(sdr-dsp): wire full DSP pipeline with condition-variable IQ queue` |
| Task 7: Update docker-compose.yml to wire sdr-dsp container | done | Committed: `chore: wire sdr-dsp container with USB passthrough and config env vars` |
| Task 8: Implement Thorough Reception Investigation (logging & IQ replay) | todo | |
| Task 9: Dynamic Settings Propagation (config.json watcher) | todo | |

## Broker & Persistence Tracker

| Task | Status | Notes |
|------|--------|-------|
| Task 1: Setup async SQLite/SQLAlchemy | done | |
| Task 2: Implement POST /messages | done | |
| Task 3: Implement GET /messages | done | |
| Task 4: Implement WebSocket broadcast | done | |
| Task 5: Periodic Message Rotation (30 days retention) | todo | |
| Task 6: Global Output Configuration (SignalK/Garmin) | todo | |

## Frontend PWA Tracker

| Task | Status | Notes |
|------|--------|-------|
| Task 1: Scaffold Vite frontend | done | Includes Docker configuration |
| Task 2: Implement Real-time Dashboard | done | |
| Task 3: Implement Filtering & Settings | done | Includes B1/B2 rules |
| Task 4: Implement SDR Config Bridge | done | Includes API extensions |
| Task 5: Enable PWA & Notifications | done | Manifest, SW, and Push |
| Task 6: Human-Readable Station/Type Mapping | todo | |
| Task 7: "Select All" / "Deselect All" Filters | todo | |
| Task 8: Separate Display vs Notification Filters | todo | |
| Task 9: System Diagnostics Tab (Logs & Metrics) | todo | |
| Task 10: Stitch-based Mobile UI Polish | todo | |

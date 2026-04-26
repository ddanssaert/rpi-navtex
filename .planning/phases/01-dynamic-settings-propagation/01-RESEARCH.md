# Phase 1: Dynamic Settings Propagation - Research

**Researched:** 2026-04-26
**Domain:** Inter-container IPC, SDRPlay API runtime control, FastAPI async HTTP
**Confidence:** HIGH

---

## Summary

Phase 1 requires that when a user saves Antenna or LNA Gain settings in the PWA, the change takes effect in the running `sdr-dsp` C++ process without restarting the DSP pipeline. Currently the `POST /config` endpoint in `api-broker` writes to `/data/config.json` and returns — the comment `# In a real app, we might signal the sdr-dsp container here` marks the gap.

The critical discovery is that the SDRPlay API v3 exposes `sdrplay_api_Update()`, which can apply antenna and gain changes to a live, streaming device with no stop/restart of the hardware. The `SoapySDRPlay3` open-source wrapper (well-established reference) demonstrates this pattern conclusively: modify the struct field, call `sdrplay_api_Update()` with the appropriate `ReasonForUpdate` flag, and the hardware applies the change inline. This means a full container restart is unnecessary and undesirable.

The recommended IPC architecture is: `api-broker` makes an async HTTP POST to a lightweight HTTP control endpoint embedded in `sdr-dsp`. `sdr-dsp` already contains a main loop and a consumer thread — it needs a third thread: a minimal HTTP control server (using cpp-httplib, header-only, zero new Docker dependencies). On receiving the POST, the control thread updates the SDRPlay params struct and calls `sdrplay_api_Update()` under the existing mutex pattern. The shared `sqlite_data` volume path `/data/config.json` already exists and is writable by both containers, but polling/inotify introduces latency and complexity; the HTTP call approach is simpler, deterministic, and consistent with how `sdr-dsp` already communicates back to `api-broker` (via HTTP POST to `/messages`).

**Primary recommendation:** Embed a minimal HTTP control server in `sdr-dsp` (cpp-httplib, port 8001, internal Docker network only); `api-broker` calls it async via `httpx` after writing config. SDRPlay settings applied live via `sdrplay_api_Update()`.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| SDRPlay API v3 `sdrplay_api_Update()` | v3.x (already vendored) | Apply antenna/gain changes to live hardware | Official API, already used in project; SoapySDRPlay3 confirms it works while streaming |
| cpp-httplib | 0.18.x (header-only) | Embedded HTTP server in sdr-dsp | Header-only, no dependencies, proven on ARM/Linux, used in many embedded projects |
| httpx | 0.27.x | Async HTTP client in api-broker | FastAPI's own test client; `AsyncClient` pattern integrates cleanly with FastAPI lifespan |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `pydantic` (already present) | 2.x | Validate config payload in api-broker | Already used for `SDRConfig` |
| `pthread` mutex (already present) | libc | Protect SDRPlay param struct from concurrent access | Already used in sdr-dsp threading |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| HTTP control endpoint | Shared `/data/config.json` + inotify/poll | File-watch is unreliable across Docker named volumes (inotify not guaranteed); polling adds 1-5s latency; HTTP is simpler and immediate |
| HTTP control endpoint | `docker restart sdr-dsp` via Docker socket | Kills USB stream, risks hardware re-enumeration, 3-10s downtime; wrong for a live appliance |
| HTTP control endpoint | ZMQ or Unix socket | ZMQ needs C++ library build step; Unix sockets need bind-mount coordination; HTTP reuses the existing pattern sdr-dsp already uses to publish messages |
| cpp-httplib | Crow, Drogon, Boost.Beast | Crow/Drogon need CMake integration changes; Boost.Beast is heavy; cpp-httplib is a single header drop-in |

**Installation (api-broker side):**
```bash
pip install httpx==0.27.*
```
Add to `api/requirements.txt`.

**Installation (sdr-dsp side):**
```bash
# No package manager needed — copy single header
curl -L https://github.com/yhirose/cpp-httplib/releases/download/v0.18.3/httplib.h \
     -o sdr-dsp/third_party/httplib.h
```
Add to `CMakeLists.txt` include path.

**Version verification:**
- httpx latest: `pip index versions httpx` → 0.27.2 confirmed current [VERIFIED: PyPI 2026-04-26]
- cpp-httplib latest release: v0.18.3 [VERIFIED: GitHub releases page 2026-04-26]

---

## Architecture Patterns

### Recommended Project Structure (changes only)
```
sdr-dsp/
├── src/
│   ├── config.h          # Add: atomic<char> g_antenna, atomic<int> g_lna_state
│   ├── control_server.h  # NEW: HTTP control server thread
│   ├── control_server.cpp# NEW: POST /control/config handler
│   └── main.cpp          # Add: control server thread startup + sdrplay_api_Update() calls

api/
└── src/
    └── main.py           # Change: POST /config calls sdr-dsp after saving file
```

### Pattern 1: SDRPlay Runtime Parameter Update
**What:** Modify the params struct, then call `sdrplay_api_Update()` with the appropriate reason flag. No hardware stop/start needed.
**When to use:** Any time antenna or gain changes while streaming is active.

```cpp
// Source: SoapySDRPlay3/Settings.cpp (open source reference, github.com/pothosware/SoapySDRPlay3)
// LNA gain state update while streaming:
{
    std::lock_guard<std::mutex> lock(_general_state_mutex);
    chParams->tunerParams.gain.LNAstate = new_lna_state;
    sdrplay_api_Update(device.dev, device.tuner,
                       sdrplay_api_Update_Tuner_Gr,
                       sdrplay_api_Update_Ext1_None);
}

// RSPdx antenna update while streaming:
{
    std::lock_guard<std::mutex> lock(_general_state_mutex);
    params->devParams->rspDxParams.antennaSel = new_antenna;
    sdrplay_api_Update(device.dev, device.tuner,
                       sdrplay_api_Update_None,
                       sdrplay_api_Update_RspDx_AntennaControl);
}
```

Key constraint: the SDRPlay params struct pointer is obtained at init via `sdrplay_api_GetDeviceParams()`. The same pointer must be used for updates — store it as a class/global member.

### Pattern 2: Control Server Thread in sdr-dsp
**What:** A dedicated thread runs a blocking HTTP server (cpp-httplib). The main streaming loop and the consumer thread are unaffected.
**When to use:** Any time external control of a long-running C++ process is needed without stop/restart.

```cpp
// Source: [ASSUMED] based on cpp-httplib README pattern (yhirose/cpp-httplib)
#include "httplib.h"

void control_server_thread(sdrplay_api_DeviceT device,
                           sdrplay_api_DeviceParamsT* params,
                           std::mutex& sdr_mutex) {
    httplib::Server svr;
    svr.Post("/control/config", [&](const httplib::Request& req,
                                    httplib::Response& res) {
        // Parse JSON body (use nlohmann/json or manual sscanf)
        // Update params struct and call sdrplay_api_Update under lock
        std::lock_guard<std::mutex> lock(sdr_mutex);
        // ... apply changes ...
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
    svr.listen("0.0.0.0", 8001);
}
```

### Pattern 3: Async HTTP Call from api-broker
**What:** After saving config.json, `POST /config` in `main.py` fires an async HTTP call to sdr-dsp's control endpoint using `httpx.AsyncClient`.
**When to use:** Caller must not block FastAPI's event loop.

```python
# Source: https://www.python-httpx.org/async/ [CITED]
import httpx

SDR_DSP_URL = "http://sdr-dsp:8001/control/config"

@app.post("/config", response_model=SDRConfig)
async def update_config(config: SDRConfig):
    save_config(config)
    try:
        async with httpx.AsyncClient(timeout=3.0) as client:
            await client.post(SDR_DSP_URL, json=config.model_dump())
    except httpx.RequestError:
        # sdr-dsp may be starting up — config.json is the fallback source of truth
        pass
    return config
```

The `try/except` is important: sdr-dsp reads config.json at startup, so even if the HTTP call fails (e.g., sdr-dsp is restarting), the saved file ensures the next startup picks up the correct values.

### Anti-Patterns to Avoid
- **Restarting the container:** `docker restart sdr-dsp` kills the USB stream and risks device re-enumeration on the RPi3B. The SDRPlay API supports live updates — use them.
- **Polling config.json in a loop:** Adds unnecessary latency (1-5s depending on poll interval) and burns CPU on the Pi. The HTTP push model is zero-latency.
- **inotify on Docker named volumes:** inotify events are not reliably delivered across Docker named volume boundaries on Linux/ARM. Multiple open bug reports confirm this (moby/moby#18246). [CITED: github.com/moby/moby/issues/18246]
- **Blocking the FastAPI event loop:** Do not use `requests.post()` (synchronous) inside an async FastAPI endpoint. Use `httpx.AsyncClient` or run in a threadpool.
- **Race condition on params struct:** The params pointer returned by `sdrplay_api_GetDeviceParams()` must be guarded by a mutex when written. The stream callback runs on a different thread.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Live SDR parameter updates | Custom reinit/restart logic | `sdrplay_api_Update()` | SDRPlay API handles the hardware negotiation; reinit risks device lockup |
| HTTP server in C++ | Raw BSD sockets + HTTP parser | cpp-httplib | HTTP parsing edge cases; cpp-httplib is 7000-line battle-tested header |
| Async HTTP in Python | `asyncio.open_connection()` raw TCP | `httpx.AsyncClient` | Connection pooling, timeout, redirect, TLS handling all built in |
| JSON parsing in C++ | Manual string search | nlohmann/json (or cpp-httplib's built-in params) | Malformed JSON from api-broker should not crash sdr-dsp |

**Key insight:** The SDRPlay API v3 was explicitly designed for runtime parameter updates during streaming. Using container restart as a workaround ignores the API's core capability.

---

## Common Pitfalls

### Pitfall 1: SDRPlay Params Pointer is Not a Copy
**What goes wrong:** Developer stores the `LNAstate` value locally and passes it to `sdrplay_api_Update()` without updating the struct behind the pointer.
**Why it happens:** `sdrplay_api_GetDeviceParams()` returns a pointer to an internal struct. The update call reads from that struct — if the struct is not modified, the hardware receives no change.
**How to avoid:** Always write to `params->rxChannelA->tunerParams.gain.LNAstate` (the struct via the pointer) before calling `sdrplay_api_Update()`.
**Warning signs:** `sdrplay_api_Update()` returns `sdrplay_api_Success` but hardware behavior does not change.

### Pitfall 2: sdrplay_api_Update Called Outside Streaming
**What goes wrong:** Update called before `sdrplay_api_Init()` or after `sdrplay_api_Uninit()`.
**Why it happens:** Control server thread starts before hardware init completes.
**How to avoid:** Use an `std::atomic<bool> g_streaming_active` flag; control endpoint returns 503 if not yet streaming.
**Warning signs:** `sdrplay_api_Update()` returns an error code indicating the device handle is invalid.

### Pitfall 3: httpx Not in requirements.txt
**What goes wrong:** api-broker container builds fine locally (httpx installed on dev machine) but fails in Docker build.
**Why it happens:** Dependency missing from `api/requirements.txt`.
**How to avoid:** Add `httpx==0.27.*` to `api/requirements.txt` before coding.

### Pitfall 4: sdr-dsp Port 8001 Not Exposed to api-broker
**What goes wrong:** HTTP call from api-broker to `http://sdr-dsp:8001` times out.
**Why it happens:** Port is only accessible within `navtex_net` bridge — this is actually fine since both containers are on the same network. No `ports:` mapping to host is needed.
**How to avoid:** Do NOT add `ports: - "8001:8001"` to docker-compose.yml — the port should be internal only. Verify with `docker exec api-broker curl http://sdr-dsp:8001/health`.
**Warning signs:** `httpx.ConnectError` in api-broker logs.

### Pitfall 5: Antenna Selection Logic is Device-Specific
**What goes wrong:** Using `sdrplay_api_Update_Rsp2_AntennaControl` on an RSPdx device (or vice versa) produces an error or is silently ignored.
**Why it happens:** The RSPdx uses `sdrplay_api_Update_Ext1` (extension flags), not the primary update flags.
**How to avoid:** The existing `main.cpp` already has the correct `hwVer` dispatch for init — mirror that same dispatch in the control server update handler.
**Warning signs:** Antenna changes not applied; `sdrplay_api_Update()` returns an unexpected error.

---

## Code Examples

### SDRPlay API Update — Verified Patterns from SoapySDRPlay3

```cpp
// Source: github.com/pothosware/SoapySDRPlay3/blob/master/Settings.cpp [CITED]

// --- LNA Gain State (works on all RSP devices) ---
params->rxChannelA->tunerParams.gain.LNAstate = (int)new_lna_state;
sdrplay_api_ErrT err = sdrplay_api_Update(
    device.dev,
    device.tuner,
    sdrplay_api_Update_Tuner_Gr,      // primary reason
    sdrplay_api_Update_Ext1_None      // extension reason
);

// --- Antenna — RSPdx / RSPdx-R2 ---
params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_A; // or B/C
sdrplay_api_Update(
    device.dev,
    device.tuner,
    sdrplay_api_Update_None,                    // primary: none
    sdrplay_api_Update_RspDx_AntennaControl     // extension: RSPdx antenna
);

// --- Antenna — RSP2 ---
params->rxChannelA->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_B;
sdrplay_api_Update(
    device.dev,
    device.tuner,
    sdrplay_api_Update_Rsp2_AntennaControl,
    sdrplay_api_Update_Ext1_None
);
```

### cpp-httplib Minimal Server

```cpp
// Source: github.com/yhirose/cpp-httplib README [CITED]
#include "httplib.h"

httplib::Server svr;
svr.Post("/control/config", handler_fn);
svr.Get("/control/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("ok", "text/plain");
});
svr.listen("0.0.0.0", 8001);
// listen() is blocking — run in a std::thread
```

### httpx Async Client in FastAPI

```python
# Source: https://www.python-httpx.org/async/ [CITED]
import httpx

async with httpx.AsyncClient(timeout=3.0) as client:
    resp = await client.post(
        "http://sdr-dsp:8001/control/config",
        json={"antenna": "A", "lna_gain": 3}
    )
    resp.raise_for_status()
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| SDRPlay API v2 (`mir_sdr_*` functions) | SDRPlay API v3 (`sdrplay_api_*`) | ~2019-2020 | v3 exposes `sdrplay_api_Update()` for clean runtime updates; v2 required reinit for most changes |
| `requests` (sync) in FastAPI | `httpx.AsyncClient` | 2022+ convention | requests blocks the event loop; httpx is fully async-native |

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | cpp-httplib is safe to use on ARM64/armv7l (Raspberry Pi 3B) | Standard Stack | May need alternative; mitigate by testing in Docker build |
| A2 | The control endpoint POST from api-broker fires synchronously within the `POST /config` handler (acceptable latency ~100ms on local Docker network) | Architecture Patterns | If sdr-dsp is slow to respond, the `/config` endpoint will feel sluggish; add timeout |
| A3 | `sdrplay_api_Update()` is safe to call from a thread other than the one that called `sdrplay_api_Init()` | Standard Stack | Could cause SDK internal race; mitigate by using the same mutex pattern as SoapySDRPlay3 |

---

## Open Questions

1. **Which RSP hardware version is in use?**
   - What we know: `main.cpp` dispatches on `hwVer` supporting RSPdx, RSPdxR2, RSP2, and RSP1/1A/1B
   - What's unclear: The specific device in deployment determines which `sdrplay_api_Update` flags to use for antenna
   - Recommendation: The control server should read `chosenDev.hwVer` and dispatch the same logic as the existing `main.cpp` antenna block

2. **Should the `/control/config` endpoint be authenticated?**
   - What we know: Port 8001 is on the internal `navtex_net` bridge, not exposed to the host
   - What's unclear: Whether internal-network-only is sufficient security for this appliance context
   - Recommendation: No auth needed — only `api-broker` can reach it on the bridge network; not exposed externally

3. **Graceful degradation if sdr-dsp is not yet ready?**
   - What we know: `sdr-dsp` takes ~3-10s to start (sdrplay_apiService + hardware init)
   - What's unclear: What the frontend should show if config save succeeds but DSP propagation fails
   - Recommendation: `POST /config` succeeds (config.json saved) even if HTTP call to sdr-dsp fails; frontend shows "saved" always; sdr-dsp reads config.json at startup as the authoritative fallback

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Docker / docker compose | Build & run | Yes | 20.10.24 / v2.35.0 | — |
| Python 3 | api-broker | Yes | 3.13.1 | — |
| httpx | api-broker async HTTP | Not in requirements.txt yet | 0.27.2 (PyPI) | — (must add) |
| cpp-httplib header | sdr-dsp control server | Not present in repo | 0.18.3 (GitHub) | — (must download) |
| SDRPlay API v3 | sdr-dsp | Yes (vendored in Docker build) | 3.x | — |

**Missing dependencies with no fallback:**
- `httpx`: Must be added to `api/requirements.txt`
- `cpp-httplib` header: Must be added to `sdr-dsp/third_party/`

---

## Validation Architecture

No `.planning/config.json` found — treating `nyquist_validation` as enabled.

### Test Framework
| Property | Value |
|----------|-------|
| Framework (api) | pytest (not yet in requirements; must add) |
| Framework (sdr-dsp) | Existing CMake test runner (`test_publisher`, etc.) |
| Quick run command | `cd api && pytest tests/ -x -q` |
| Full suite command | `cd api && pytest tests/ && cd ../sdr-dsp && cmake --build build && ctest --test-dir build` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROP-01 | `POST /config` saves config.json | unit | `pytest tests/test_config.py::test_post_config_saves -x` | No — Wave 0 |
| PROP-02 | `POST /config` calls sdr-dsp control endpoint | unit (httpx mock) | `pytest tests/test_config.py::test_post_config_notifies_dsp -x` | No — Wave 0 |
| PROP-03 | sdr-dsp control endpoint accepts `{"antenna":"A","lna_gain":3}` and returns 200 | integration/smoke | `curl -s -X POST http://localhost:8001/control/config -d '{"antenna":"A","lna_gain":0}'` | No — Wave 0 |
| PROP-04 | Settings survive sdr-dsp restart (config.json read at startup) | manual smoke | Restart container, confirm settings persist | Manual |

### Wave 0 Gaps
- [ ] `api/tests/test_config.py` — covers PROP-01, PROP-02
- [ ] `api/tests/conftest.py` — shared FastAPI test client fixture
- [ ] Add `pytest==8.*` and `pytest-asyncio==0.23.*` and `respx==0.21.*` to `api/requirements.txt` (for mocking httpx)
- [ ] cpp-httplib header at `sdr-dsp/third_party/httplib.h`

---

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | Internal Docker network only; no auth surface |
| V3 Session Management | No | Stateless REST |
| V4 Access Control | Partial | Port 8001 must NOT be mapped to host in docker-compose.yml |
| V5 Input Validation | Yes | Pydantic `SDRConfig` validates antenna (str) and lna_gain (int) in api-broker before forwarding |
| V6 Cryptography | No | Internal LAN only; HTTPS not warranted for control endpoint on bridge network |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed JSON to sdr-dsp control endpoint | Tampering | Use nlohmann/json with try/catch; return 400 on parse error |
| Antenna value out of range (e.g., "Z") | Tampering | Validate in api-broker (Pydantic) before forwarding; also validate in C++ handler |
| LNA state out of valid range (device-dependent) | Tampering | Clamp to 0-8 in both api-broker and sdr-dsp; SDRPlay API may crash on invalid values |
| Port 8001 accidentally exposed to internet | Elevation of privilege | Never add `ports:` for 8001 in docker-compose.yml; document this explicitly |

---

## Sources

### Primary (HIGH confidence)
- [SoapySDRPlay3/Settings.cpp](https://github.com/pothosware/SoapySDRPlay3/blob/master/Settings.cpp) — setGain() and setAntenna() runtime update patterns; mutex usage; streaming-safe calls
- [cpp-httplib README](https://github.com/yhirose/cpp-httplib) — embedded HTTP server pattern
- [httpx async docs](https://www.python-httpx.org/async/) — AsyncClient usage in FastAPI

### Secondary (MEDIUM confidence)
- [SDRplay API Specification v3.08 PDF](https://www.sdrplay.com/docs/SDRplay_API_Specification_v3.08.pdf) — PDF not machine-readable in this session; content inferred from SoapySDRPlay3 source and community sources
- [SoapySDRPlay3 Issue #58](https://github.com/pothosware/SoapySDRPlay3/issues/58) — confirmed `sdrplay_api_Update(Tuner_Gr)` can time out under pathological conditions; warrants timeout/guard in implementation

### Tertiary (LOW confidence)
- [moby/moby Issue #18246](https://github.com/moby/moby/issues/18246) — inotify unreliable on Docker named volumes (supports ruling out file-watch IPC)
- WebSearch results on Docker volume file watching — consistent with the named-volume inotify limitation finding

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — SDRPlay runtime API confirmed via SoapySDRPlay3 source; httpx and cpp-httplib are standard well-documented libraries
- Architecture: HIGH — follows same HTTP pattern already used by sdr-dsp→api-broker; no novel patterns
- Pitfalls: MEDIUM — pitfall 3 (params pointer) and pitfall 5 (device-specific flags) are confirmed from source code; others are standard Docker/threading hygiene

**Research date:** 2026-04-26
**Valid until:** 2026-07-26 (stable domain; SDRPlay API versions change slowly)

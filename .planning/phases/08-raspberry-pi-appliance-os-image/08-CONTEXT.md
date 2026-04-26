# Phase 8: Raspberry Pi Appliance OS Image - Context

**Gathered:** 2026-04-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Produce a single flashable `.img.xz` for Raspberry Pi 3B (BCM2837, arm64, Bookworm) with the navtex Docker stack pre-installed. The device boots headless into the running stack with zero post-flash configuration. WiFi credentials, SSH, and hostname are injected at flash time via Raspberry Pi Imager's OS Customisation flow. A NetworkManager fallback hotspot (`Navtex-AP`) activates automatically when no known WiFi is reachable.

This phase does NOT include:
- Web admin UI for network configuration (future phase)
- Remote OTA update mechanism (future phase)
- Multi-WiFi network configuration via UI (future phase)
- Read-only rootfs (incompatible with Docker overlay2)

</domain>

<decisions>
## Implementation Decisions

### Docker Image Embedding
- **D-01:** Build all container images (sdr-dsp, api-broker, pwa-frontend) inside pi-gen's QEMU arm64 chroot in the custom `stage-navtex` stage. `docker build` runs for each service during image build; resulting images are saved into the rootfs so `docker compose up` on first boot requires no internet pull.
- **D-02:** If sdr-dsp C++ compilation exceeds CI time limits under QEMU emulation, fall back to a two-stage pipeline: cross-compile arm64 images via `docker buildx --platform linux/arm64` on x86, save with `docker save`, embed tarballs into the OS image via a pi-gen `COPY_FILES` step instead. Start with QEMU; adapt if needed.

### Image Distribution
- **D-03:** CI publishes the final `navtex.img.xz` (plus sha256 checksum) as a GitHub Releases asset. A companion `rpi-imager.json` manifest is also published alongside the image. Users paste the manifest URL (or a stable `latest` redirect URL) into RPi Imager's "Add a custom repository" field — this enables the OS Customisation dialog for WiFi/SSH/hostname injection at flash time.
- **D-04:** CI image build triggers on version tag pushes only (`v*`). The ~30-minute build time makes per-commit builds impractical. Use `pi-gen-action` (usimd/pi-gen-action@v1) as the GitHub Actions integration.

### Network Mode
- **D-05:** The docker-compose.yml is updated to use `network_mode: host` for all three containers (sdr-dsp, api-broker, pwa-frontend). Containers bind directly to the RPi host's ports. The `HOST_IP` environment variable is removed. `BROKER_URL` for sdr-dsp changes to `http://localhost:8000/messages`. Port mapping entries (`ports:`) are removed. This is the standard pattern for RPi appliances and eliminates the hardcoded IP address problem.

### Hotspot Access Control
- **D-06:** The `Navtex-AP` fallback hotspot is an **open AP** (no WPA password). The NM connection profile uses `key-mgmt=none`. This minimises friction for first-time setup — the user can connect without looking up a password.

### Claude's Discretion
- pi-gen config values: locale, keyboard layout, timezone defaults (use en_GB / Europe/London as research suggests; user can override via Imager customisation)
- Default hostname baked into the image (`navtex` → resolves as `navtex.local`)
- Exact systemd unit configuration for Docker Compose autostart
- AccessPopup non-interactive installation method in the chroot
- avahi-daemon configuration details
- GitHub Actions workflow structure beyond the trigger (steps, artifact naming)
- JSON manifest field values (extract_size, image_download_size, devices list)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Existing Compose Stack (must be updated to host networking)
- `docker-compose.yml` — Current bridge-network configuration. D-05 requires updating all services to `network_mode: host`, removing `HOST_IP` env var, removing `ports:` entries, changing `BROKER_URL` to `http://localhost:8000/messages`.

### Existing API (understand entry points)
- `api/src/main.py` — API service structure; relevant for understanding startup behaviour and the SDR DSP → API communication path.

### Phase Research (primary technical reference)
- `.planning/phases/08-raspberry-pi-appliance-os-image/08-RESEARCH.md` — Comprehensive technical research: pi-gen architecture, code examples, pitfalls, AccessPopup pattern, Imager customisation mechanism, Docker-in-chroot approach. Planner MUST read this.

### No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `docker-compose.yml`: Base for the host-network version. All three services (sdr-dsp, api-broker, pwa-frontend) and the `sqlite_data` volume carry forward — only networking configuration changes.
- Existing Dockerfiles (`api/Dockerfile`, `frontend/Dockerfile`, `sdr-dsp/Dockerfile`): Will be used as-is inside the pi-gen chroot for `docker build`. No changes anticipated.

### Established Patterns
- Docker Compose restart policy: currently `on-failure` for sdr-dsp. Research recommends `unless-stopped` for appliance use.
- Volume: `sqlite_data` named volume persists data across container restarts — unchanged.

### Integration Points
- New `image/` directory at repo root: pi-gen config file, `stage-navtex/` custom stage, `build.sh` wrapper.
- New `.github/workflows/build-image.yml`: CI pipeline for tagged releases.
- `docker-compose.yml`: Updated in-place (host networking, no HOST_IP).

</code_context>

<specifics>
## Specific Ideas

- The phase goal names the hotspot SSID as `Navtex-AP` exactly — use this verbatim in the NM profile.
- mDNS hostname: `navtex.local` — baked in via pi-gen `TARGET_HOSTNAME=navtex` and avahi-daemon.
- The JSON manifest must include `"init_format": "systemd"` for RPi Imager OS Customisation to work (see RESEARCH.md Pattern 3).
- Do not enable raspi-config's overlay filesystem — it conflicts with Docker's overlay2 storage driver (RESEARCH.md Pitfall 5).

</specifics>

<deferred>
## Deferred Ideas

- Web admin UI for WiFi configuration — future phase
- Multiple WiFi network support via NM connection profiles — future phase (NM supports this; scaffold deferred)
- OTA update mechanism — future phase
- Read-only rootfs — blocked by Docker incompatibility (RESEARCH.md Pitfall 5)

</deferred>

---

*Phase: 08-raspberry-pi-appliance-os-image*
*Context gathered: 2026-04-26*

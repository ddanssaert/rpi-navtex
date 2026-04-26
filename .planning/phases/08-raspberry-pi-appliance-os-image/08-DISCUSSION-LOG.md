# Phase 8: Raspberry Pi Appliance OS Image - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-26
**Phase:** 08-raspberry-pi-appliance-os-image
**Areas discussed:** Docker image embedding, Image distribution, Network mode, Hotspot access control

---

## Docker image embedding

| Option | Description | Selected |
|--------|-------------|----------|
| Build in QEMU chroot | pi-gen custom stage runs `docker build` for all containers inside the arm64 chroot. Simple pipeline, no extra tooling. Risk: sdr-dsp C++ compile may be slow under QEMU. | ✓ |
| Cross-compile with docker buildx | Build arm64 images on x86, `docker save`, embed tarballs. Faster CI, more complex pipeline. | |

**User's choice:** Build in QEMU chroot (recommended default)

**Follow-up — fallback if CI times out:**

| Option | Description | Selected |
|--------|-------------|----------|
| Switch to buildx | Automatically fall back to cross-compile path if QEMU build is too slow. | ✓ |
| Pre-built sdr-dsp only | Cross-compile only the slow sdr-dsp; keep others in QEMU. | |
| Accept longer CI time | No CI time constraint. Local builds only. | |

**User's choice:** Switch to buildx (recommended — adapt if needed)

---

## Image distribution

| Option | Description | Selected |
|--------|-------------|----------|
| GitHub Releases + JSON manifest | CI publishes image + manifest. Users paste manifest URL into Imager for OS Customisation dialog. Best UX. | ✓ |
| GitHub Releases only | Plain download. OS Customisation dialog stays grayed out. Users configure manually. | |
| Local build only | No CI. Developer-only distribution. | |

**User's choice:** GitHub Releases + JSON manifest (recommended)

**Follow-up — CI trigger:**

| Option | Description | Selected |
|--------|-------------|----------|
| On version tags only | Triggers on `v*` tag pushes. ~30 min build makes per-commit impractical. | ✓ |
| Tags + manual dispatch | Tag releases plus GitHub Actions "Run workflow" button. | |
| Manual dispatch only | Only when explicitly triggered. | |

**User's choice:** On version tags only (recommended)

---

## Network mode

| Option | Description | Selected |
|--------|-------------|----------|
| Switch to host networking | `network_mode: host` for all containers. Bind directly to host ports. Remove `HOST_IP`. `BROKER_URL` → `http://localhost:8000/messages`. | ✓ |
| Keep bridge, use service name | Fix URL to use Docker Compose service name. Keep port mappings. Slightly more isolated. | |

**User's choice:** Switch to host networking (recommended)

---

## Hotspot access control

| Option | Description | Selected |
|--------|-------------|----------|
| WPA2 password: 'navtex-setup' | Password printed in README. Prevents accidental auto-connections. | |
| Open AP (no password) | Anyone nearby can connect. Zero friction for first-time setup. | ✓ |

**User's choice:** Open AP (no password)

---

## Claude's Discretion

- pi-gen locale/timezone defaults
- Exact systemd unit configuration
- AccessPopup non-interactive install method
- avahi-daemon configuration
- GitHub Actions workflow structure
- JSON manifest field values

## Deferred Ideas

- Web admin UI for WiFi configuration
- Multiple WiFi network support
- OTA update mechanism
- Read-only rootfs (Docker incompatible)

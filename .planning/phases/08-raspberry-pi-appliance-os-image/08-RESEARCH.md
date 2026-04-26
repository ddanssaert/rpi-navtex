# Phase 8: Raspberry Pi Appliance OS Image - Research

**Researched:** 2026-04-26
**Domain:** Embedded Linux image building / Raspberry Pi OS / Docker on ARM / NetworkManager hotspot
**Confidence:** MEDIUM-HIGH (core mechanisms verified via official docs; some configuration details from community sources)

---

## Summary

This phase produces a single flashable `.img.xz` for a Raspberry Pi 3B running the navtex Docker stack headlessly with zero post-flash configuration. The user flashes via Raspberry Pi Imager, enters WiFi/SSH/hostname in the OS Customisation dialog, and the device boots directly into the running stack.

The OS image build ecosystem has changed significantly in 2025. The official `pi-gen` tool remains the most stable and battle-tested path for producing a Raspberry Pi OS Lite image with custom additions. A newer official tool, `rpi-image-gen`, was released in March 2025 but targets Raspberry Pi 4/5 with 64-bit OS only and has experimental status — it is not suitable for RPi 3B with the required stability. `pi-gen` is the correct choice. [CITED: raspberrypi.com/news/introducing-rpi-image-gen]

The RPi Imager OS Customisation mechanism also changed in 2025. Imager 2.0 switched from the legacy `firstrun.sh` shell script to cloud-init. However, the legacy `firstrun.sh` mechanism still works for Bookworm (Debian 12) images, and most users run Imager 1.9.x. The safest path for maximum Imager compatibility is to build a Bookworm-based image that supports the `firstrun.sh` (`init_format: "systemd"`) mechanism, which works with both Imager 1.9.x and 2.0. The cloud-init path (`init_format: "cloudinit-rpi"`) requires Imager 2.0 and Trixie (Debian 13). [CITED: raspberrypi.com/news/cloud-init-on-raspberry-pi-os]

The fallback hotspot pattern is well-solved by the `AccessPopup` project, which uses NetworkManager's native AP mode (no hostapd/dnsmasq standalone) with a systemd timer that checks every 2 minutes. This approach is compatible with the BCM43438 chip in the RPi 3B and works on Bookworm. [CITED: raspberryconnect.com/projects/65-raspberrypi-hotspot-accesspoints/203]

**Primary recommendation:** Build a Bookworm arm64 image with pi-gen (stages 0–2 + custom stage), using `firstrun.sh` for Imager compatibility, NetworkManager AccessPopup for hotspot fallback, and a systemd unit for Docker Compose autostart. Skip read-only rootfs for now (Docker incompatibility is a significant blocker).

---

## Standard Stack

### Core Build Tools
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| `pi-gen` | latest (arm64 branch) | Build RPi OS image | Official Raspberry Pi tool, battle-tested, stage-based customization |
| `pi-gen-action` | v1 | GitHub Actions wrapper for pi-gen | Only maintained GHA integration for pi-gen [CITED: github.com/usimd/pi-gen-action] |
| Docker (build host) | 20.10+ | pi-gen Docker build isolation | Avoids host-contamination, works on Debian/Ubuntu CI runners |

### Target OS Packages
| Package | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `docker-ce` + `docker-compose-plugin` | 29.x (arm64) | Container runtime + Compose | Official Docker repo; Compose v2 as plugin [CITED: docs.docker.com/engine/install/raspberry-pi-os] |
| `avahi-daemon` | distro default | mDNS for `navtex.local` | Included in RPi OS Lite, resolves `.local` on all clients without DNS config |
| `networkmanager` | distro default | WiFi + hotspot management | Replaces dhcpcd in Bookworm; required for AP mode without hostapd |
| `dnsmasq-base` | distro default | DHCP for hotspot clients | NetworkManager hotspot dependency (not standalone dnsmasq) |

### Distribution Note
**Use arm64 (64-bit) Bookworm, not armhf (32-bit).** RPi 3B's Cortex-A53 is ARMv8 and runs arm64. armhf security updates stopped in late 2023; Docker Engine v29+ drops armhf support. [CITED: docs.docker.com/engine/install/raspberry-pi-os] [VERIFIED: WebSearch community sources]

### Installation (in pi-gen custom stage chroot):
```bash
# Add Docker apt repo (run inside chroot)
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc
chmod a+r /etc/apt/keyrings/docker.asc
echo "deb [arch=arm64 signed-by=/etc/apt/keyrings/docker.asc] \
  https://download.docker.com/linux/debian bookworm stable" \
  > /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

---

## Architecture Patterns

### Recommended Repository Structure
```
rpi-navtex/
├── image/                       # pi-gen build artifacts and config
│   ├── config                   # pi-gen config file (IMG_NAME, LOCALE, etc.)
│   ├── stage-navtex/            # Custom pi-gen stage
│   │   ├── 00-packages          # Packages to install (avahi-daemon, etc.)
│   │   ├── 01-install/
│   │   │   ├── 00-run-chroot.sh # Docker install, user setup
│   │   │   └── files/           # Systemd unit, docker-compose.yml, hotspot config
│   │   └── 02-hotspot/
│   │       └── 00-run-chroot.sh # AccessPopup install + NM connection profile
│   └── build.sh                 # Wrapper: clones pi-gen, injects stage, runs build
├── .github/workflows/
│   └── build-image.yml          # pi-gen-action CI workflow
└── docker-compose.yml           # Prod compose (network_mode: host)
```

### Pattern 1: pi-gen Custom Stage

**What:** Add a directory `stage-navtex/` after stage2 in the STAGE_LIST. pi-gen runs numbered subdirectories in order. `00-run-chroot.sh` scripts execute inside the target rootfs chroot.

**When to use:** All OS-level configuration — Docker install, systemd units, NetworkManager profiles, copying compose files.

**Example (config file):**
```bash
# image/config
IMG_NAME="navtex"
RELEASE="bookworm"
TARGET_HOSTNAME="navtex"
FIRST_USER_NAME="navtex"
FIRST_USER_PASS=""          # disabled; SSH key auth only
ENABLE_SSH=1
STAGE_LIST="stage0 stage1 stage2 stage-navtex"
USE_QEMU=1
```

**Example (stage-navtex/01-install/00-run-chroot.sh):**
```bash
#!/bin/bash -e
# Source: pi-gen documentation pattern
# Install Docker Engine
install -m 0755 -d /etc/apt/keyrings
# ... (full Docker install commands as above)
usermod -aG docker navtex

# Copy navtex stack files
install -d /opt/navtex
cp /tmp/docker-compose.yml /opt/navtex/docker-compose.yml

# Enable navtex systemd service
systemctl enable navtex.service
```
[CITED: github.com/RPi-Distro/pi-gen]

### Pattern 2: Systemd Service for Docker Compose Autostart

**What:** A systemd unit in `/etc/systemd/system/navtex.service` that runs `docker compose up` after Docker and network-online.target are ready.

**When to use:** Always — this is the standard pattern for Docker Compose on boot. [CITED: multiple community sources cross-verified]

```ini
# /etc/systemd/system/navtex.service
[Unit]
Description=Navtex Docker Stack
Requires=docker.service
After=docker.service network-online.target
Wants=network-online.target

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/opt/navtex
ExecStart=/usr/bin/docker compose up -d --wait
ExecStop=/usr/bin/docker compose down
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

### Pattern 3: RPi Imager Compatibility (firstrun.sh / init_format)

**What:** For a custom image to support OS Customisation in RPi Imager, the image's JSON manifest entry must declare `"init_format": "systemd"`. Imager then writes a `firstrun.sh` script to the boot partition, referencing it in `cmdline.txt` as `init=/usr/lib/raspberrypi-sys-mods/firstboot`. The OS runs this script on first boot to set hostname, user, WiFi, and SSH. [CITED: raspberrypi.com/news/how-to-add-your-own-images-to-imager]

**When to use:** For hosting image on a custom JSON manifest so Imager's Advanced Options dialog is enabled.

**Image JSON manifest entry:**
```json
{
  "name": "Navtex Appliance Image",
  "description": "RPi 3B Navtex receiver — headless, zero-config",
  "url": "https://github.com/yourorg/rpi-navtex/releases/latest/download/navtex.img.xz",
  "extract_size": 3800000000,
  "extract_sha256": "<sha256>",
  "image_download_size": 600000000,
  "image_download_sha256": "<sha256>",
  "release_date": "2026-04-26",
  "init_format": "systemd",
  "devices": ["pi3"]
}
```

**Key requirement:** The image must be built from RPi OS (not Ubuntu), must include `raspberrypi-sys-mods` package, and the boot partition must be FAT32. All of these are satisfied by a standard pi-gen stage2 output.

**Compatibility note:** When using "Use custom" (local file selection) in Imager, the Advanced Options dialog does NOT appear regardless of image content. The dialog only appears when the image is selected from a JSON manifest (online or custom repo URL). [VERIFIED: WebSearch community sources]

### Pattern 4: NetworkManager Fallback Hotspot (AccessPopup)

**What:** A systemd timer (`AccessPopup.timer`) runs the `accesspopup` script every 2 minutes. If no known WiFi SSID is reachable, the script activates an NM connection profile in AP mode. When a known SSID returns, it switches back. Uses NM's built-in hotspot — no standalone hostapd or dnsmasq required. `dnsmasq-base` (Debian) must be installed for NM to provide DHCP to hotspot clients. [CITED: raspberryconnect.com/projects/65-raspberrypi-hotspot-accesspoints/203]

**When to use:** Always — this is the required fallback hotspot mechanism.

**Key files installed by AccessPopup:**
- `/usr/bin/accesspopup` — main monitoring script
- `/etc/accesspopup.conf` — SSID, password, IP config
- `/etc/NetworkManager/system-connections/AccessPopup.nmconnection` — NM AP profile
- systemd timer + service units

**Installation in pi-gen chroot:**
```bash
# Clone and install AccessPopup in chroot
git clone https://github.com/RaspberryConnect/AccessPopup.git /tmp/accesspopup
cd /tmp/accesspopup
# Non-interactive install: copy script, create NM profile, enable timer
# Set SSID="Navtex-AP" and password in /etc/accesspopup.conf
```

**NM connection profile for the AP:**
```ini
[connection]
id=Navtex-AP
type=wifi
autoconnect=false

[wifi]
mode=ap
ssid=Navtex-AP

[wifi-security]
key-mgmt=wpa-psk
psk=navtex2024

[ipv4]
method=shared
address1=192.168.99.1/24
```

**RPi 3B / BCM43438 compatibility:** Confirmed working on Bookworm with NetworkManager. WPA2 (not WPA3) required for the AP profile on BCM43438. [CITED: raspberryconnect.com, community forums]

### Pattern 5: Docker network_mode: host

**What:** All three containers use `network_mode: host` instead of bridge networking. The `HOST_IP` env var is eliminated; `BROKER_URL` changes to `http://localhost:8000/messages`. Port mappings (`ports:`) are removed as containers bind directly to host ports. [ASSUMED — user-stated preference; aligns with standard practice for RPi appliances]

**docker-compose.yml (host network version):**
```yaml
version: '3.8'
services:
  sdr-dsp:
    build: ./sdr-dsp
    container_name: sdr-dsp
    restart: unless-stopped
    network_mode: host
    privileged: true
    devices:
      - /dev/bus/usb:/dev/bus/usb
    volumes:
      - sqlite_data:/data
    environment:
      - SDR_ANTENNA=A
      - SDR_LNA_STATE=0
      - BROKER_URL=http://localhost:8000/messages
    depends_on:
      - api-broker

  api-broker:
    build: ./api
    container_name: api-broker
    network_mode: host
    volumes:
      - sqlite_data:/data

  pwa-frontend:
    build: ./frontend
    container_name: pwa-frontend
    network_mode: host
    volumes:
      - sqlite_data:/data:ro
    depends_on:
      - api-broker

volumes:
  sqlite_data:
```

**Note:** `depends_on` still works with `network_mode: host` for startup ordering. `privileged: true` on sdr-dsp is unchanged and still needed for USB device access.

### Anti-Patterns to Avoid

- **Building images on the RPi itself:** pi-gen runs on x86 Linux with QEMU, not on the target. Using the target as a build host defeats reproducibility.
- **Using `rpi-image-gen` for RPi 3B:** rpi-image-gen targets arm64 Pi 4/5; lacks stable RPi 3B support and is under active development. [CITED: raspberrypi.com/news/introducing-rpi-image-gen]
- **Using armhf (32-bit):** Security updates stopped late 2023; Docker v29+ drops armhf. Always use arm64. [VERIFIED: Docker docs + community sources]
- **Standalone hostapd + dnsmasq for hotspot:** NetworkManager's built-in AP mode is supported natively on BCM43438 in Bookworm. Standalone hostapd conflicts with NM.
- **Enabling raspi-config overlay filesystem with Docker:** `/var/lib/docker` write access is incompatible with the overlay tmpfs approach. The cross-device-link error kills Docker. [CITED: grafolean.medium.com + community forums]
- **Pulling images at first boot:** Pre-bake Docker images into the image build (via `docker pull` + `docker save`/`docker load` in the custom stage), or build from source. Internet dependency at first boot violates zero-config requirement.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| RPi OS image building | Custom dd/debootstrap script | `pi-gen` (stages 0–2 + custom) | Handles boot partition, firmware, raspi-config, package sources correctly |
| GitHub Actions image CI | Custom workflow scripting | `pi-gen-action` (usimd) | Handles QEMU binfmt registration, disk space, artifact upload [CITED: github.com/usimd/pi-gen-action] |
| WiFi fallback hotspot switching | Custom nm-dispatcher script | `AccessPopup` project | Handles edge cases: timer-based scanning, graceful AP↔client switching, RPi-specific NM quirks [CITED: raspberryconnect.com] |
| mDNS hostname resolution | Custom DNS | `avahi-daemon` (already in RPi OS Lite) | Zero config; resolves `navtex.local` on Windows/macOS/Linux clients automatically |
| Docker Compose autostart | Custom init script | systemd unit with `After=docker.service network-online.target` | Standard pattern; handles restart, dependency ordering, journald logging |
| Image compression | Manual xz | pi-gen's `DEPLOY_COMPRESSION=xz` config var | Produces standard `.img.xz` RPi Imager understands |

**Key insight:** The RPi ecosystem has well-maintained tooling for every problem in this phase. Custom scripts for image building, hotspot switching, or service management will re-encounter known edge cases these tools already handle.

---

## Common Pitfalls

### Pitfall 1: Imager OS Customisation Grayed Out for Local Files
**What goes wrong:** When the user selects a local `.img.xz` file via "Use custom" in RPi Imager, the Advanced Options / OS Customisation button does not appear, regardless of what's in the image.
**Why it happens:** Imager only offers customisation when loading image metadata from a JSON manifest (online or custom repo URL). Local files have no associated metadata.
**How to avoid:** Provide a JSON manifest file users can add as a custom repository URL in Imager, OR accept that users must manually edit `firstrun.sh` / cloud-init files on the boot partition after flashing.
**Warning signs:** Users report the gear icon is grayed out after flashing.

### Pitfall 2: Docker Image Pull at First Boot Fails (No Internet)
**What goes wrong:** If `docker compose up` runs before internet access and images haven't been pre-loaded, the service fails with "manifest unknown" or pull timeout.
**Why it happens:** The navtex stack builds custom images from source. If the Dockerfiles reference base images not already present, Docker attempts to pull them.
**How to avoid:** In the pi-gen custom stage, after installing Docker, build the images inside the chroot (or use `docker save`/`docker load`) so they are baked into the SD card image.
**Warning signs:** `journalctl -u navtex.service` shows pull errors on first boot.

### Pitfall 3: AccessPopup / NetworkManager AP Mode Fails on Bookworm
**What goes wrong:** Hotspot activation times out or DHCP fails for connected clients.
**Why it happens:** (a) Standalone `dnsmasq.service` conflicts with `dnsmasq-base`; (b) WPA3 is specified instead of WPA2; (c) WiFi power saving causes interface drops.
**How to avoid:** Ensure `dnsmasq.service` is not installed (only `dnsmasq-base`); set `key-mgmt=wpa-psk` (WPA2) in the AP NM profile; add `wifi.powersave=2` (disable) to the connection profile.
**Warning signs:** `journalctl -u NetworkManager` shows "Hotspot network creation took too long".

### Pitfall 4: pi-gen Build Fails with "Previous stage rootfs not found"
**What goes wrong:** pi-gen Docker build fails on resume or re-run with missing stage cache.
**Why it happens:** Docker layer caching is incomplete; stage tarballs in `work/` directory are missing.
**How to avoid:** Use `CONTINUE=1` when resuming; don't clean `work/` between builds; pin the pi-gen git revision for reproducibility.
**Warning signs:** `build.sh` error output referencing missing `rootfs.tar`.

### Pitfall 5: Overlay FS Breaks Docker
**What goes wrong:** After enabling `raspi-config`'s overlay filesystem feature, Docker fails to start with `invalid cross-device link` or `rename /var/lib/docker/runtimes`.
**Why it happens:** Docker's overlay2 storage driver conflicts with overlayfs-on-tmpfs. `/var/lib/docker` is on a tmpfs, and Docker's rename operations cross filesystem boundaries.
**How to avoid:** Do not enable the raspi-config overlay filesystem feature in an image that runs Docker. For SD card resilience, prefer frequent backups or use a high-endurance SD card instead. [CITED: grafolean.medium.com + community forums]
**Warning signs:** `docker info` shows storage driver errors after reboot.

### Pitfall 6: arm64 Image Baked for RPi 4+ Fails on RPi 3B
**What goes wrong:** Image boots to kernel panic or fails to start due to incompatible device tree.
**Why it happens:** Some arm64 RPi OS images include optimizations or device trees targeting BCM2711 (Pi 4) but not BCM2837 (Pi 3B).
**How to avoid:** Use `pi-gen` with the `arm64` branch targeting Bookworm — it explicitly supports BCM2837. Avoid `rpi-image-gen` (targets Pi 4/5 only). [CITED: github.com/RPi-Distro/pi-gen/tree/arm64]

### Pitfall 7: network-online.target Races Docker Compose
**What goes wrong:** Docker Compose starts before WiFi is associated, container networking fails, and the service enters a failure loop.
**Why it happens:** `network-online.target` is reached when any interface is up — ethernet may satisfy it even if WiFi hasn't joined.
**How to avoid:** Add `Wants=network-online.target` AND `After=network-online.target`. Enable `systemd-networkd-wait-online.service` or use `NetworkManager-wait-online.service`. Consider `Restart=on-failure` with `RestartSec=10` as a safety net.

---

## Code Examples

### pi-gen config file
```bash
# image/config
# Source: pi-gen README [CITED: github.com/RPi-Distro/pi-gen]
IMG_NAME="navtex"
RELEASE="bookworm"
TARGET_HOSTNAME="navtex"
FIRST_USER_NAME="navtex"
DISABLE_FIRST_BOOT_USER_RENAME=1
ENABLE_SSH=1
LOCALE_DEFAULT="en_GB.UTF-8"
KEYBOARD_KEYMAP="gb"
KEYBOARD_LAYOUT="English (UK)"
TIMEZONE_DEFAULT="Europe/London"
STAGE_LIST="stage0 stage1 stage2 stage-navtex"
USE_QEMU=1
DEPLOY_COMPRESSION="xz"
```

### pi-gen stage skip setup (skip stages 3-5)
```bash
# Source: pi-gen README [CITED: github.com/RPi-Distro/pi-gen]
touch stage3/SKIP stage4/SKIP stage5/SKIP
touch stage4/SKIP_IMAGES stage5/SKIP_IMAGES
```

### GitHub Actions workflow (pi-gen-action)
```yaml
# Source: github.com/usimd/pi-gen-action [CITED]
name: Build Navtex Image
on:
  push:
    tags: ['v*']
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: usimd/pi-gen-action@v1
        with:
          image-name: navtex
          pi-gen-version: arm64
          stage-list: "stage0 stage1 stage2 ./image/stage-navtex"
          pi-gen-dir: /tmp/pi-gen
          increase-runner-disk-size: true
      - uses: actions/upload-artifact@v4
        with:
          name: navtex-image
          path: deploy/*.img.xz
```

### NetworkManager connection profile for Navtex-AP
```ini
# /etc/NetworkManager/system-connections/Navtex-AP.nmconnection
# Source: raspberryconnect.com AccessPopup + nmcli docs [CITED]
[connection]
id=Navtex-AP
uuid=<generated>
type=wifi
autoconnect=false
interface-name=wlan0

[wifi]
mode=ap
ssid=Navtex-AP

[wifi-security]
key-mgmt=wpa-psk
psk=navtex-setup

[ipv4]
method=shared
address1=192.168.99.1/24

[ipv6]
method=disabled
```

### avahi-daemon hostname advertisement
```ini
# /etc/avahi/avahi-daemon.conf (ensure this is set)
# Source: avahi documentation [ASSUMED]
[server]
host-name=navtex
domain-name=local
use-ipv4=yes
use-ipv6=no
allow-interfaces=wlan0
```

### SHA256 checksum generation for distribution
```bash
# Source: standard Linux tooling [VERIFIED: tooling exists on all Debian systems]
sha256sum navtex.img.xz > navtex.img.xz.sha256
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `wpa_supplicant.conf` + `dhcpcd.conf` for WiFi | NetworkManager with `nmcli` | Bookworm (2023) | Old config files ignored on Bookworm; must use NM |
| `firstrun.sh` for RPi Imager customisation | cloud-init (`user-data`, `network-config`) | Trixie / Imager 2.0 (Nov 2025) | Bookworm still uses firstrun.sh; Trixie uses cloud-init |
| `docker-compose` v1 standalone | `docker compose` v2 plugin | 2023 | `docker-compose` binary deprecated; use `docker compose` |
| armhf (32-bit) as default RPi OS | arm64 (64-bit) recommended | Bookworm (2023) | armhf security updates ended; Docker v29+ drops armhf |
| hostapd + dnsmasq for AP mode | NetworkManager built-in AP mode | Bookworm (2023) | NM AP mode is reliable on BCM43438; hostapd conflicts with NM |
| `rpi-image-gen` for custom images | pi-gen remains the stable tool | 2025 (rpi-image-gen experimental) | rpi-image-gen targets Pi 4/5 only; pi-gen is correct for Pi 3B |

**Deprecated/outdated:**
- `wpa_supplicant.conf` on boot partition: ignored in Bookworm (NetworkManager manages WiFi)
- `dhcpcd.conf`: replaced by NetworkManager in Bookworm
- `ssh` empty file on boot partition: still works but `ENABLE_SSH=1` in pi-gen config is cleaner
- `userconf.txt`: still works but superseded by pi-gen's `FIRST_USER_NAME` config

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Switching to `network_mode: host` is the correct fix for the hardcoded `HOST_IP` issue | Architecture Patterns (Pattern 5) | If wrong, bridge networking with dynamic `HOST_IP` discovery is needed instead |
| A2 | AccessPopup can be installed non-interactively inside a pi-gen chroot (no GUI) | Architecture Patterns (Pattern 4) | Would need a different hotspot solution or manual post-flash config |
| A3 | Docker images for the navtex stack can be built inside a pi-gen QEMU chroot | Common Pitfalls (Pitfall 2) | If too slow or hits QEMU limits, an alternative pre-load strategy is needed |
| A4 | avahi-daemon is sufficient for `navtex.local` without additional config | Standard Stack | Windows clients may need Bonjour/mDNS support installed separately |
| A5 | The navtex stack base images (python, nginx, gcc) have arm64 variants on Docker Hub | Don't Hand-Roll | If any image is amd64-only, Dockerfile must be changed to build from source |

---

## Open Questions

1. **Docker images in pi-gen chroot — QEMU build speed**
   - What we know: pi-gen uses QEMU user-mode emulation for arm64 in chroot. Building C++ with QEMU can take hours.
   - What's unclear: Whether building the `sdr-dsp` C++ container inside QEMU chroot is feasible within CI time limits.
   - Recommendation: Investigate building images via cross-compilation on x86 + `docker buildx`, then using `docker save` to embed in the image. May need to split image build from OS image build.

2. **SDRPlay driver in Docker**
   - What we know: sdr-dsp uses the SDRPlay API. The current Dockerfile is not in this repository context.
   - What's unclear: Whether the SDRPlay API library requires kernel modules on the host OS, or whether it works with only USB device passthrough.
   - Recommendation: Confirm `/dev/bus/usb` passthrough + `privileged: true` is sufficient, or identify which kernel modules need loading on the host.

3. **Imager Customisation with locally-distributed images**
   - What we know: Local `.img.xz` files don't show the Customisation dialog in Imager.
   - What's unclear: Whether the project should host a JSON manifest (GitHub Releases + manifest URL) or accept a simpler distribution where users manually configure via the "Advanced Options" workaround.
   - Recommendation: For phase scope, distribute via GitHub Releases with a companion JSON manifest URL that users paste into Imager's "Add a custom repository" field.

4. **Multiple WiFi networks**
   - What we know: User identified this as nice-to-have, not required for phase 8.
   - What's unclear: Whether the phase should scaffold the NM connection file structure to support multiple networks from day one.
   - Recommendation: Defer to the WebUI phase; NM supports multiple connection profiles and they can be added via `nmcli` which the WebUI can wrap.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Docker | pi-gen Docker build | ✓ | 20.10.24 | — |
| Docker Compose | Stack deployment | ✓ | v2.35.0 | — |
| Git | pi-gen clone | ✓ | 2.39.5 | — |
| Python 3 | pi-gen scripts | ✓ | 3.13.1 | — |
| QEMU (`qemu-user-static`) | pi-gen arm64 chroot | ✗ | — | Install via `apt-get install qemu-user-static binfmt-support` or `docker run --privileged tonistiigi/binfmt --install all` |
| GitHub Actions (ubuntu-latest) | CI image build | ✓ (cloud) | — | Local build with Docker |

**Missing dependencies with no fallback:** None blocking.

**Missing dependencies with fallback:**
- QEMU (`qemu-user-static`): Not installed on build host. pi-gen Docker build registers binfmt via the build container automatically when using `build-docker.sh`. On GitHub Actions, `pi-gen-action` handles this. For local development, install via `sudo apt-get install qemu-user-static binfmt-support`.

---

## Validation Architecture

No automated test framework is applicable to OS image building — the validation is:
1. **Smoke test:** Flash image to SD card, boot, confirm `navtex.local` resolves and the WebUI loads.
2. **Hotspot test:** Boot without known WiFi, confirm `Navtex-AP` SSID appears within 3 minutes.
3. **Imager test:** Load JSON manifest URL in RPi Imager, confirm OS Customisation dialog appears, fill in WiFi/hostname/SSH, flash, boot with zero post-flash config.

These are manual tests. No Wave 0 test infrastructure needed.

### Validation Checklist (Phase Gate)
- [ ] Image file produced: `navtex-<date>-arm64.img.xz`
- [ ] SHA256 checksum file produced alongside image
- [ ] `firstrun.sh` present on boot partition after flash
- [ ] `docker compose ps` shows all 3 services Up after first boot
- [ ] `curl http://navtex.local` returns 200 OK
- [ ] `Navtex-AP` SSID visible when no known WiFi present
- [ ] RPi Imager shows OS Customisation dialog when image loaded from JSON manifest

---

## Security Domain

This phase is an infrastructure/build phase, not a web API phase. ASVS categories V2/V3/V4 are not applicable. Relevant considerations:

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | no | No user-facing API in this phase |
| V6 Cryptography | partial | SSH public key auth (not password); no custom crypto |

**Key security items:**
- Default user password should be disabled; SSH key auth only (configured via Imager OS Customisation).
- Hotspot AP WPA2 password should be a reasonable default (not empty); users can change via WebUI later.
- Docker socket is exposed to the `navtex` user via `docker` group — this is equivalent to root access. Acceptable for a single-user appliance.
- `privileged: true` on sdr-dsp is a broad privilege grant. It's required for USB device access with the SDRPlay API. Acceptable for a dedicated appliance but note in documentation.

---

## Sources

### Primary (HIGH confidence)
- [Cloud-init on Raspberry Pi OS](https://www.raspberrypi.com/news/cloud-init-on-raspberry-pi-os/) — firstrun.sh vs cloud-init transition, init_format values, boot partition files
- [How to add your own images to Imager](https://www.raspberrypi.com/news/how-to-add-your-own-images-to-imager/) — init_format field, JSON manifest schema, custom image requirements
- [pi-gen GitHub](https://github.com/RPi-Distro/pi-gen) — stage structure, SKIP files, custom stage patterns
- [Docker Engine on Raspberry Pi OS](https://docs.docker.com/engine/install/raspberry-pi-os/) — arm64 vs armhf, install commands, package names
- [Introducing rpi-image-gen](https://www.raspberrypi.com/news/introducing-rpi-image-gen-build-highly-customised-raspberry-pi-software-images/) — experimental status, Pi 4/5 focus

### Secondary (MEDIUM confidence)
- [AccessPopup — Automated WiFi/AP switching](https://www.raspberryconnect.com/projects/65-raspberrypi-hotspot-accesspoints/203-automated-switching-accesspoint-wifi-network) — NM hotspot approach, BCM43438 compatibility, WPA2 requirement
- [pi-gen-action GitHub](https://github.com/usimd/pi-gen-action) — GitHub Actions integration, runner requirements
- [Running RPi with read-only root filesystem](https://www.dzombak.com/blog/2024/03/running-a-raspberry-pi-with-a-read-only-root-filesystem/) — overlay FS patterns, Docker incompatibility
- [Run Docker on RPi read-only filesystem](https://grafolean.medium.com/run-docker-on-your-raspberry-pi-read-only-file-system-raspbian-1360cf94bace) — cross-device link error, /var/lib/docker tmpfs workaround

### Tertiary (LOW confidence — marked ASSUMED in text)
- Various Raspberry Pi Forums posts on NM hotspot issues, WPA3 vs WPA2, BCM43438 behavior

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Docker install docs official; pi-gen official; AccessPopup community-verified
- Architecture: MEDIUM — pi-gen stage patterns from official README; firstrun.sh mechanism from official Raspberry Pi news posts; systemd unit pattern community-standard
- Pitfalls: MEDIUM — overlay FS + Docker incompatibility from multiple cross-verified sources; some NM hotspot issues from forum posts only
- Image distribution format: HIGH — .img.xz is the standard RPi Imager format; SHA256 is universal practice

**Research date:** 2026-04-26
**Valid until:** 2026-07-26 (90 days — pi-gen is stable; rpi-image-gen evolving rapidly but not in scope)

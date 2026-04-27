#!/bin/bash -e

# 1) Layout Docker repo files directly in rootfs (host side)
install -m 0755 -d "${ROOTFS_DIR}/etc/apt/keyrings"
curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o "${ROOTFS_DIR}/etc/apt/keyrings/docker.gpg"
chmod a+r "${ROOTFS_DIR}/etc/apt/keyrings/docker.gpg"

echo "deb [arch=arm64 signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/debian bookworm stable" \
  > "${ROOTFS_DIR}/etc/apt/sources.list.d/docker.list"

# 2) Install systemd units and config files (host side)
install -m 0644 files/navtex.service "${ROOTFS_DIR}/etc/systemd/system/navtex.service"
install -m 0644 files/avahi-daemon.conf "${ROOTFS_DIR}/etc/avahi/avahi-daemon.conf"

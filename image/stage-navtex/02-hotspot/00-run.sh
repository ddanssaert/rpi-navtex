#!/bin/bash -e

# 1) Clone AccessPopup on the host to avoid chroot networking/proxy issues
AP_HOST_DIR="/tmp/accesspopup-host-$(date +%s)"
git clone --depth 1 https://github.com/RaspberryConnect/AccessPopup.git "$AP_HOST_DIR"

# 2) Install the accesspopup script directly to rootfs (host side)
# The browser subagent confirmed it is in the root of the repo.
install -m 0755 "$AP_HOST_DIR/accesspopup" "${ROOTFS_DIR}/usr/bin/accesspopup"

# Cleanup host temp dir
rm -rf "$AP_HOST_DIR"

# 3) Layout NetworkManager profiles (host side)
install -d -m 0700 "${ROOTFS_DIR}/etc/NetworkManager/system-connections"
install -m 0600 files/Navtex-AP.nmconnection \
                "${ROOTFS_DIR}/etc/NetworkManager/system-connections/Navtex-AP.nmconnection"

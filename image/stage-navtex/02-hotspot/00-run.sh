#!/bin/bash -e

# 1) Layout NetworkManager profiles (host side)
install -d -m 0700 "${ROOTFS_DIR}/etc/NetworkManager/system-connections"
install -m 0600 files/Navtex-AP.nmconnection \
                "${ROOTFS_DIR}/etc/NetworkManager/system-connections/Navtex-AP.nmconnection"

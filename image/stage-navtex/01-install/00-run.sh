#!/bin/bash -e

# Copy the service and config files to the rootfs /tmp so the chroot script can install them.
install -m 644 files/navtex.service "${ROOTFS_DIR}/tmp/"
install -m 644 files/avahi-daemon.conf "${ROOTFS_DIR}/tmp/"

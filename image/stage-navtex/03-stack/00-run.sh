#!/bin/bash -e

# Copy the navtex source tree to the rootfs /tmp so the chroot script can access it.
# We use -a to preserve permissions and -r for recursive.
cp -a files/navtex-src "${ROOTFS_DIR}/tmp/"

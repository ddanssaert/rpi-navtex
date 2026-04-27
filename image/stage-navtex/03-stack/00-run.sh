#!/bin/bash -e

# Copy the navtex source tree to the rootfs /navtex-src-staging
# (Avoid /tmp because pi-gen mounts a tmpfs over it in on_chroot)
mkdir -p "${ROOTFS_DIR}/navtex-src-staging"
cp -a files/navtex-src/* "${ROOTFS_DIR}/navtex-src-staging/"

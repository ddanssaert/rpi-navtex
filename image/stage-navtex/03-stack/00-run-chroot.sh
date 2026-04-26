#!/bin/bash -e
# Bake navtex Docker images into the rootfs (D-01) with D-02 tarball fallback.
#
# Strategy:
#   1) If pre-saved image tarballs exist in /tmp (copied from stage files/), load them.
#   2) Otherwise, copy the navtex source from /pi-gen-host (mounted by build.sh /
#      pi-gen-action workspace) and run `docker compose build` inside the chroot.
#
# Either path produces the three images present in the local docker engine, baked
# into /var/lib/docker on the rootfs so first boot needs NO internet.

# Start dockerd in the chroot for the duration of this script.
dockerd --iptables=false --bridge=none > /var/log/dockerd-bake.log 2>&1 &
DOCKERD_PID=$!
# Wait for dockerd socket.
for i in $(seq 1 30); do
  if docker info >/dev/null 2>&1; then break; fi
  sleep 1
done
if ! docker info >/dev/null 2>&1; then
  echo "ERROR: dockerd failed to start in chroot" >&2
  cat /var/log/dockerd-bake.log >&2
  exit 1
fi

# --- Path B (D-02 fallback): pre-saved tarballs --------------------------------
TARBALLS=$(ls /tmp/*.tar 2>/dev/null || true)
if [ -n "$TARBALLS" ]; then
  echo "D-02 fallback: loading pre-saved image tarballs"
  for t in $TARBALLS; do
    docker load -i "$t"
  done
else
  # --- Path A (D-01): Pull from registry in chroot ------------------------------
  echo "D-01: pulling navtex images from GHCR inside QEMU arm64 chroot"
  if [ -d /tmp/navtex-src ]; then
    SRC=/tmp/navtex-src
  elif [ -d /pi-gen/stage-navtex/03-stack/files/navtex-src ]; then
    SRC=/pi-gen/stage-navtex/03-stack/files/navtex-src
  else
    echo "ERROR: navtex source tree not found in chroot."
    exit 1
  fi

  # Use the production compose file for the pull
  cp "$SRC/docker-compose.prod.yml" /tmp/docker-compose.yml
  docker compose -f /tmp/docker-compose.yml pull
fi

# Drop the production compose file at /opt/navtex
install -m 0644 /tmp/docker-compose.yml /opt/navtex/docker-compose.yml
chown navtex:navtex /opt/navtex/docker-compose.yml

# Stop dockerd cleanly
kill "$DOCKERD_PID"
wait "$DOCKERD_PID" 2>/dev/null || true
sync

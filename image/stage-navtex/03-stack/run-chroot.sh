#!/bin/bash -e
# Bake navtex Docker images into the rootfs (D-01)

# Strategy:
#   1) Start dockerd.
#   2) Pull images from GHCR using docker-compose.prod.yml.
#   3) Layout the final compose file in /opt/navtex.

# Start dockerd in the chroot for the duration of this script.
dockerd --iptables=false --bridge=none > /var/log/dockerd-bake.log 2>&1 &
DOCKERD_PID=$!
# Wait for dockerd socket.
for i in $(seq 1 30); do
  if docker info >/dev/null 2>&1; then break; fi
  sleep 1
done

# --- Path A (D-01): Pull from registry in chroot ------------------------------
SRC=/navtex-src-staging
if [ ! -d "$SRC" ]; then
  echo "ERROR: navtex source tree not found in chroot at $SRC"
  exit 1
fi

# Use the production compose file for the pull
cp "$SRC/docker-compose.prod.yml" /opt/navtex/docker-compose.yml
docker compose -f /opt/navtex/docker-compose.yml pull

# Cleanup dockerd
kill "$DOCKERD_PID"
wait "$DOCKERD_PID" 2>/dev/null || true
sync

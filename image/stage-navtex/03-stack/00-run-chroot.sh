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
  # --- Path A (D-01): build in chroot ------------------------------------------
  echo "D-01: building navtex images inside QEMU arm64 chroot"
  # The pi-gen-action workflow mounts the repo at /pi-gen/stage-navtex; for local
  # build.sh runs the wrapper bind-mounts the repo root into /tmp/navtex-src.
  if [ -d /tmp/navtex-src ]; then
    SRC=/tmp/navtex-src
  elif [ -d /pi-gen/stage-navtex/03-stack/files/navtex-src ]; then
    SRC=/pi-gen/stage-navtex/03-stack/files/navtex-src
  else
    echo "ERROR: navtex source tree not found in chroot. Provide either:"
    echo "  - /tmp/navtex-src bind mount, OR"
    echo "  - pre-saved tarballs in image/stage-navtex/03-stack/files/*.tar"
    exit 1
  fi

  cp "$SRC/docker-compose.yml" /tmp/docker-compose.yml
  cd "$SRC"
  docker compose -f docker-compose.yml build sdr-dsp api-broker pwa-frontend
fi

# Drop the host-networked compose file at /opt/navtex (the chroot script in
# 01-install/ already created the directory and made navtex:navtex the owner).
install -m 0644 /tmp/docker-compose.yml /opt/navtex/docker-compose.yml
chown navtex:navtex /opt/navtex/docker-compose.yml

# Stop dockerd cleanly so the storage layer flushes.
kill "$DOCKERD_PID"
wait "$DOCKERD_PID" 2>/dev/null || true
sync

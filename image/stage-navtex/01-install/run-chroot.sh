#!/bin/bash -e
# Runs inside QEMU arm64 chroot with target rootfs as /.

# 1) Install Docker CE from the official Docker apt repo
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
    docker-ce docker-ce-cli containerd.io \
    docker-buildx-plugin docker-compose-plugin

# 2) Add the navtex first-user to the docker group.
usermod -aG docker navtex

# 3) Create the navtex stack working directory; the docker-compose.yml is dropped
#    in by stage 03-stack/ which also embeds the built images.
install -d -o navtex -g navtex /opt/navtex

# 4) Enable services so they autostart on first boot.
systemctl enable docker.service
systemctl enable avahi-daemon.service
systemctl enable NetworkManager-wait-online.service
systemctl enable navtex.service

# 5) Disable wifi powersave globally (RESEARCH Pitfall 3).
install -d /etc/NetworkManager/conf.d
cat > /etc/NetworkManager/conf.d/10-disable-powersave.conf <<'EOF'
[connection]
wifi.powersave = 2
EOF

# 6) Ensure standalone dnsmasq.service is masked if present (Pitfall 3).
systemctl mask dnsmasq.service 2>/dev/null || true

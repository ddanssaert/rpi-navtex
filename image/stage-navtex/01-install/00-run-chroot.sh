#!/bin/bash -e
# Runs inside QEMU arm64 chroot with target rootfs as /.

# 1) Install Docker CE from the official Docker apt repo (RESEARCH §Standard Stack).
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc
chmod a+r /etc/apt/keyrings/docker.asc
echo "deb [arch=arm64 signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian bookworm stable" \
  > /etc/apt/sources.list.d/docker.list
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
    docker-ce docker-ce-cli containerd.io \
    docker-buildx-plugin docker-compose-plugin

# 2) Add the navtex first-user to the docker group.
usermod -aG docker navtex

# 3) Install systemd unit (file provided via stage files/ → /tmp).
install -d /etc/systemd/system
install -m 0644 /tmp/navtex.service /etc/systemd/system/navtex.service

# 4) Configure avahi for navtex.local.
install -m 0644 /tmp/avahi-daemon.conf /etc/avahi/avahi-daemon.conf

# 5) Create the navtex stack working directory; the docker-compose.yml is dropped
#    in by stage 03-stack/ which also embeds the built images.
install -d -o navtex -g navtex /opt/navtex

# 6) Enable services so they autostart on first boot.
systemctl enable docker.service
systemctl enable avahi-daemon.service
systemctl enable NetworkManager-wait-online.service
systemctl enable navtex.service

# 7) Disable wifi powersave globally (RESEARCH Pitfall 3).
install -d /etc/NetworkManager/conf.d
cat > /etc/NetworkManager/conf.d/10-disable-powersave.conf <<'EOF'
[connection]
wifi.powersave = 2
EOF

# 8) Ensure standalone dnsmasq.service is masked if present (Pitfall 3).
systemctl mask dnsmasq.service 2>/dev/null || true

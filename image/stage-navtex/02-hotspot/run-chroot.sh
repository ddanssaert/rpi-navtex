#!/bin/bash -e
# Install AccessPopup non-interactively and configure for the open Navtex-AP.

# 1) Clone AccessPopup into a temp location inside the chroot.
git clone --depth 1 https://github.com/RaspberryConnect/AccessPopup.git /tmp/AccessPopup

# 2) Install the accesspopup script + systemd units manually.
install -m 0755 /tmp/AccessPopup/installconfig/accesspopup /usr/bin/accesspopup

# 3) AccessPopup config — SSID Navtex-AP, NO PSK (open AP per D-06).
cat > /etc/accesspopup.conf <<'EOF'
# AccessPopup configuration
# D-06: Navtex-AP is an OPEN access point. Leave ap_pw blank.
ap_ssid='Navtex-AP'
ap_pw=''
ap_ip='192.168.99.1'
ap_gw='192.168.99.1'
ap_sn='255.255.255.0'
ap_dhcp_range='192.168.99.50,192.168.99.150,12h'
EOF

# 4) Systemd timer + service for periodic check (every 2 minutes).
cat > /etc/systemd/system/accesspopup.service <<'EOF'
[Unit]
Description=AccessPopup hotspot fallback check
After=NetworkManager.service
Wants=NetworkManager.service

[Service]
Type=oneshot
ExecStart=/usr/bin/accesspopup -a
EOF

cat > /etc/systemd/system/accesspopup.timer <<'EOF'
[Unit]
Description=Run AccessPopup every 2 minutes

[Timer]
OnBootSec=30s
OnUnitActiveSec=2min
Unit=accesspopup.service

[Install]
WantedBy=timers.target
EOF

# 5) Enable the timer.
systemctl enable accesspopup.timer

# 6) Cleanup.
rm -rf /tmp/AccessPopup

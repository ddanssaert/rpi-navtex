#!/usr/bin/env bash
# Local build wrapper. CI uses usimd/pi-gen-action and reads ./config + ./stage-navtex directly.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PI_GEN_DIR="${PI_GEN_DIR:-/tmp/pi-gen}"
PI_GEN_REF="${PI_GEN_REF:-arm64}"

if [ ! -d "$PI_GEN_DIR" ]; then
  git clone --branch "$PI_GEN_REF" --depth 1 https://github.com/RPi-Distro/pi-gen.git "$PI_GEN_DIR"
fi

# Skip stages 3-5 (we only need a Lite-base + custom stage).
touch "$PI_GEN_DIR/stage3/SKIP" "$PI_GEN_DIR/stage4/SKIP" "$PI_GEN_DIR/stage5/SKIP"
touch "$PI_GEN_DIR/stage4/SKIP_IMAGES" "$PI_GEN_DIR/stage5/SKIP_IMAGES"

# Symlink our stage and config into pi-gen.
ln -sfn "$HERE/stage-navtex" "$PI_GEN_DIR/stage-navtex"
cp -f "$HERE/config" "$PI_GEN_DIR/config"

# Stage navtex source for the 03-stack build (D-01)
# This matches the strategy in .github/workflows/build-image.yml
echo "Staging source code into stage-navtex/03-stack/files/navtex-src..."
mkdir -p "$HERE/stage-navtex/03-stack/files/navtex-src"
rsync -a --delete --exclude='.git' --exclude='image' --exclude='.planning' \
      --exclude='node_modules' --exclude='deploy' --exclude='work' \
      "$HERE/../" "$HERE/stage-navtex/03-stack/files/navtex-src/"
cp "$HERE/stage-navtex/03-stack/files/navtex.service" /tmp/navtex.service

cd "$PI_GEN_DIR"
echo "Starting pi-gen build-docker.sh (requires sudo)..."
sudo ./build-docker.sh

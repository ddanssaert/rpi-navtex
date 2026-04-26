#!/usr/bin/env bash
# Generate rpi-imager.json from rpi-imager.json.template after a successful build.
# Required env: TAG, REPO (e.g. "yourorg/rpi-navtex"), IMG_PATH (path to navtex.img.xz)
set -euo pipefail

: "${TAG:?TAG required (e.g. v0.1.0)}"
: "${REPO:?REPO required (e.g. owner/rpi-navtex)}"
: "${IMG_PATH:?IMG_PATH required (path to navtex.img.xz)}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE="$HERE/../rpi-imager.json.template"
OUT="${OUT:-$HERE/../../rpi-imager.json}"

# Compressed (.img.xz) — what the user downloads.
DOWNLOAD_SIZE=$(stat -c%s "$IMG_PATH")
DOWNLOAD_SHA256=$(sha256sum "$IMG_PATH" | awk '{print $1}')

# Uncompressed — Imager needs this to size the SD card and verify post-write.
EXTRACT_SIZE=$(xz --robot --list "$IMG_PATH" | awk '/^totals/ {print $5}')
EXTRACT_SHA256=$(xz -dc "$IMG_PATH" | sha256sum | awk '{print $1}')

RELEASE_DATE=$(date -u +%Y-%m-%d)

sed \
  -e "s|__TAG__|${TAG}|g" \
  -e "s|__REPO__|${REPO}|g" \
  -e "s|__EXTRACT_SIZE__|${EXTRACT_SIZE}|g" \
  -e "s|__EXTRACT_SHA256__|${EXTRACT_SHA256}|g" \
  -e "s|__DOWNLOAD_SIZE__|${DOWNLOAD_SIZE}|g" \
  -e "s|__DOWNLOAD_SHA256__|${DOWNLOAD_SHA256}|g" \
  -e "s|__RELEASE_DATE__|${RELEASE_DATE}|g" \
  "$TEMPLATE" > "$OUT"

# Verify it parses as JSON.
jq . "$OUT" > /dev/null

echo "Generated $OUT"
echo "  download size:    $DOWNLOAD_SIZE"
echo "  download sha256:  $DOWNLOAD_SHA256"
echo "  extract size:     $EXTRACT_SIZE"
echo "  extract sha256:   $EXTRACT_SHA256"

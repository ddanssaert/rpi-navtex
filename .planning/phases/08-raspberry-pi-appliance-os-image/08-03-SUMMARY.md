# Phase 08-03 Summary: CI Release Pipeline & Distribution

Successfully implemented the CI pipeline and RPi Imager manifest system.

## Components:
-   **GitHub Actions Workflow**: `.github/workflows/build-image.yml` triggers on `v*` tags, builds the image using `pi-gen-action`, and uploads artifacts to GitHub Releases.
-   **RPi Imager Manifest**: `image/rpi-imager.json.template` and `image/scripts/generate-manifest.sh` produce a manifest that enables RPi Imager's "OS Customisation" dialog.
-   **Release Assets**: Each release includes `navtex.img.xz`, its SHA256 sidecar, and `rpi-imager.json`.

## Distribution Handle:
Users can add the Navtex appliance to RPi Imager by pasting the following URL into "Add a custom repository":
`https://github.com/<owner>/rpi-navtex/releases/latest/download/rpi-imager.json`

## Build Performance:
-   **Timeout**: Set to 90 minutes (typical builds take 30-40 minutes).
-   **Cleanup**: Workflow includes a step to free runner disk space (removes Android, .NET, etc.) to accommodate the large image workspace.

The Phase 8 implementation is complete and ready for its first release tag.

#!/usr/bin/env bash
# Package an already-built OpenKJ.app in a read-only macOS disk image.
# Run on macOS: ./packaging/create_dmg.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="${OPENKJ_APP:-$ROOT/dist/OpenKJ.app}"
OUTDIR="${OPENKJ_RELEASE_DIR:-$ROOT/releases}"

[[ "$(uname -s)" == "Darwin" ]] || { echo 'ERROR: This script requires macOS (hdiutil).' >&2; exit 1; }
[[ -d "$APP/Contents" ]] || { echo "ERROR: App bundle not found: $APP" >&2; exit 1; }
[[ -f "$APP/Contents/Info.plist" ]] || { echo "ERROR: Missing Info.plist in $APP" >&2; exit 1; }
for cmd in hdiutil ditto codesign shasum; do
  command -v "$cmd" >/dev/null || { echo "ERROR: Missing command: $cmd" >&2; exit 1; }
done

# Prefer the actual Info.plist version so the file matches the tested binary.
VERSION="${OPENKJ_VERSION:-}"

if [[ -z "$VERSION" ]]; then
  VERSION=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP/Contents/Info.plist" 2>/dev/null || true)
fi

if [[ -z "$VERSION" ]]; then
  VERSION=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$APP/Contents/Info.plist" 2>/dev/null || true)
fi

[[ -n "$VERSION" ]] || {
  echo 'ERROR: No bundle version found. Set OPENKJ_VERSION explicitly.' >&2
  exit 1
}
[[ "$VERSION" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] || { echo "ERROR: Unsafe version string: $VERSION" >&2; exit 1; }

mkdir -p "$OUTDIR"
DMG="$OUTDIR/OpenKJ-${VERSION}-macos-arm64.dmg"
STAGE="$(mktemp -d "${TMPDIR:-/tmp}/openkj-dmg.XXXXXXXX")"
cleanup() { rm -rf "$STAGE"; }
trap cleanup EXIT

echo '[1/5] Verify the app signature'
codesign --verify --deep --strict --verbose=2 "$APP"

echo '[2/5] Stage the drag-to-Applications contents'
ditto "$APP" "$STAGE/OpenKJ.app"
ln -s /Applications "$STAGE/Applications"

echo '[3/5] Create compressed read-only DMG'
# A plain Finder layout: app plus Applications shortcut. No external utilities.
hdiutil create -quiet -ov -format UDZO -imagekey zlib-level=9 \
  -volname "OpenKJ $VERSION" -srcfolder "$STAGE" "$DMG"

echo '[4/5] Verify disk image integrity'
hdiutil verify -quiet "$DMG"

echo '[5/5] SHA-256 checksum'
shasum -a 256 "$DMG"
du -h "$DMG"
printf '\nSUCCESS: %s\n' "$DMG"
echo 'NOTE: This DMG is ad-hoc signed only; it is NOT Developer ID signed or notarized.'

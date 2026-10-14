#!/bin/bash
# Build and ad-hoc sign the local Apple Silicon OpenKJ.app.
# Run from any directory; creates/overwrites <repo>/dist/OpenKJ.app.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD="$ROOT/build-macos-dev"
APP="$ROOT/dist/OpenKJ.app"
SOURCE_APP="$BUILD/openkj.app"
GST_PREFIX="$(brew --prefix gstreamer)"
QT_PREFIX="$(brew --prefix qt)"
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
GST_SOURCE="$GST_PREFIX/lib/gstreamer-1.0"
LOCAL_SOUNDTOUCH="$ROOT/build-macos-deps/soundtouch-1.28.8/ext/soundtouch/libgstsoundtouch.dylib"
if [[ -f "$LOCAL_SOUNDTOUCH" ]]; then
  SOUNDTOUCH_PLUGIN="${SOUNDTOUCH_PLUGIN:-$LOCAL_SOUNDTOUCH}"
else
  # Preserve the original developer setup until setup_macos.sh has been run.
  SOUNDTOUCH_PLUGIN="${SOUNDTOUCH_PLUGIN:-$HOME/Developer/gstreamer-build/gstreamer/build-soundtouch-standalone/ext/soundtouch/libgstsoundtouch.dylib}"
fi

for file in "$MACDEPLOYQT" "$SOUNDTOUCH_PLUGIN" "$GST_PREFIX/libexec/gstreamer-1.0/gst-plugin-scanner"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file" >&2; exit 1; }
done

echo '[1/8] Build OpenKJ'
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  -DOPENKJ_SOUNDTOUCH_PLUGIN="$SOUNDTOUCH_PLUGIN"
cmake --build "$BUILD" --parallel 4
[[ -d "$SOURCE_APP" ]] || { echo "Missing compiled app: $SOURCE_APP" >&2; exit 1; }

echo '[2/8] Create clean dist copy'
mkdir -p "$ROOT/dist"
rm -rf "$APP"
ditto "$SOURCE_APP" "$APP"

echo '[3/8] Deploy Qt and linked libraries'
"$MACDEPLOYQT" "$APP" -verbose=1 -no-strip -no-codesign
# Optional Qt plugins with unavailable optional Qt modules on this Homebrew build.
rm -f "$APP/Contents/PlugIns/imageformats/libqpdf.dylib"
rm -f "$APP/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib"

echo '[4/8] Bundle GStreamer plugins'
DEST="$APP/Contents/PlugIns"
mkdir -p "$DEST" "$APP/Contents/Helpers" "$APP/Contents/Frameworks"
# Intentionally flat: the previously tested macOS codesign layout.
plugins=(
  libgstcoreelements.dylib libgstplayback.dylib libgstaudioconvert.dylib
  libgstcdg.dylib libgstaudiofx.dylib libgstequalizer.dylib
  libgstvolume.dylib libgstautodetect.dylib libgstapp.dylib
  libgstisomp4.dylib libgstosxaudio.dylib libgsttypefindfunctions.dylib
  libgstaudioparsers.dylib libgstlibav.dylib libgstvideoparsersbad.dylib
  libgstvideoconvertscale.dylib libgstaudioresample.dylib libgstid3demux.dylib
)
for plugin in "${plugins[@]}"; do
  [[ -f "$GST_SOURCE/$plugin" ]] || { echo "Missing GStreamer plugin: $plugin" >&2; exit 1; }
  ditto "$GST_SOURCE/$plugin" "$DEST/$plugin"
done
ditto "$SOUNDTOUCH_PLUGIN" "$DEST/libgstsoundtouch.dylib"
ditto "$GST_PREFIX/libexec/gstreamer-1.0/gst-plugin-scanner" "$APP/Contents/Helpers/gst-plugin-scanner"

echo '[5/8] Relocate recursive third-party dependencies'
python3 "$ROOT/packaging/relocate_macos.py" "$APP"

echo '[6/8] Check for previously observed Qt plugin resolution problems'
# macdeployqt may copy plugins whose optional frameworks are not installed.
# The relocator checks Homebrew load dependencies; signing verification comes next.

echo '[7/8] Sign inside-out with local ad-hoc signature'
# Sign standalone dylibs and helper executables, leaving framework bundles
# to be signed as a unit below. Symlinks are intentionally skipped.
while IFS= read -r -d '' binary; do
  case "$binary" in
    *.framework/*) continue ;;
  esac
  if file -b "$binary" | grep -q 'Mach-O'; then
    codesign --force --sign - "$binary"
  fi
done < <(find "$APP/Contents/Frameworks" "$APP/Contents/PlugIns" "$APP/Contents/Helpers" -type f -print0)

# Framework bundles must be signed after their contents are finalized.
while IFS= read -r -d '' framework; do
  codesign --force --sign - "$framework"
done < <(find "$APP/Contents/Frameworks" -maxdepth 1 -type d -name '*.framework' -print0)

codesign --force --sign - --identifier org.openkj.openkj "$APP"

echo '[8/8] Verify bundle signature'
codesign --verify --deep --strict --verbose=2 "$APP"
du -sh "$APP"
echo "SUCCESS: $APP"
echo 'Finder test: open "'"$APP"'"'

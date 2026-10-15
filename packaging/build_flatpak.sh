#!/usr/bin/env bash
# Build an installable preview from committed sources, then test its runtime.
set -euo pipefail
cd "$(dirname "$0")/.."
[[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || {
    echo 'Build this preview on Linux x86_64.' >&2; exit 1;
}
for tool in git tar flatpak flatpak-builder python3; do
    command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done
scope=(--user)
if [[ ${OPENKJ_FLATPAK_SYSTEM:-0} == 1 ]]; then scope=(); fi
flatpak "${scope[@]}" remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Do not copy local build trees, media, databases or caches into the sandbox.
rm -rf build-flatpak-source
mkdir build-flatpak-source
git archive --format=tar HEAD --output=build-flatpak-source.tar
tar -xf build-flatpak-source.tar -C build-flatpak-source
flatpak-builder "${scope[@]}" --install-deps-from=flathub --force-clean --disable-rofiles-fuse \
    --default-branch=preview --repo=build-flatpak-repo --install \
    build-flatpak-app packaging/flatpak/org.openkj.OpenKJ.json

# Test the installed/exported app using the Platform runtime, not the SDK.
# No host GStreamer paths or previously cached registry may mask missing plugins.
env -u GST_PLUGIN_PATH -u GST_PLUGIN_PATH_1_0 -u GST_PLUGIN_SYSTEM_PATH -u GST_PLUGIN_SYSTEM_PATH_1_0 \
    flatpak run "${scope[@]}" --env=QT_QPA_PLATFORM=offscreen \
    --env=GST_REGISTRY_1_0=/tmp/openkj-flatpak-check-registry.bin \
    --command=/app/libexec/openkj-runtime-smoke org.openkj.OpenKJ//preview

mkdir -p releases/flatpak
flatpak build-bundle --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo \
    build-flatpak-repo releases/flatpak/OpenKJ-Linux-x86_64-preview.flatpak org.openkj.OpenKJ preview
python3 - <<'PY'
import hashlib, json, pathlib, subprocess
folder = pathlib.Path('releases/flatpak')
bundle = folder / 'OpenKJ-Linux-x86_64-preview.flatpak'
digest = hashlib.file_digest(bundle.open('rb'), 'sha256').hexdigest()
bundle.with_suffix('.flatpak.sha256').write_text(f'{digest}  {bundle.name}\n')
manifest = json.loads(pathlib.Path('packaging/flatpak/org.openkj.OpenKJ.json').read_text())
manifest['openkj-source-commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
(folder / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
flatpak "${scope[@]}" list --runtime > releases/flatpak/runtime-versions.txt
cp packaging/flatpak/README.md releases/flatpak/README.md

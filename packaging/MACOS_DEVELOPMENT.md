# Apple Silicon development and releases

This is the setup for `master`. Use a native Apple Silicon terminal;
Intel and Rosetta builds are not covered. No files from the original developer's
computer are needed, and application settings and song databases are not copied.

## First setup

Install Apple's Command Line Tools (`xcode-select --install`) and
[native Homebrew](https://brew.sh). Then:

```bash
git clone --branch master https://github.com/EquinoxEarth/OpenKJ.git
cd OpenKJ
./packaging/setup_macos.sh
source build-macos-deps/env.sh
cmake --build build-macos-dev --parallel 4
open build-macos-dev/openkj.app
```

Setup installs the formulas in `packaging/Brewfile.macos`, builds the unmodified
GStreamer 1.28.8 SoundTouch plugin from a checksum-verified source archive, tests
pitch/EQ processing, and configures a RelWithDebInfo build. CMake fetches the
pinned spdlog, libzip and TagLib versions. Setup can be rerun; it reuses the source
download and build directory and does not implicitly upgrade installed formulas.
Use `--skip-brew` when dependencies are already installed, or `--no-configure`
to prepare dependencies without configuring the application.

Homebrew dependencies follow the versions available for the host macOS; this is
a repeatable setup process, **not a byte-for-byte dependency lock**. Exact tool
and formula versions are recorded in `build-macos-deps/environment.txt` for
comparing machines and diagnosing CI failures. GStreamer must support the 1.28
plugin source. Dependency upgrades still need playback testing.

`build-macos-deps/` and `build-macos-dev/` are local, ignored outputs. Rerun setup
after moving the checkout. The script does not modify shell profiles. In each
new terminal, source `build-macos-deps/env.sh` before building. For a CMake IDE,
use `build-macos-dev` as the build directory; setup has already saved Qt and
SoundTouch paths in its CMake cache.

## Build checks and standalone bundle

```bash
source build-macos-deps/env.sh
cmake -S . -B build-macos-dev -DOPENKJ_BUILD_SMOKE_TESTS=ON
cmake --build build-macos-dev --parallel 4
ctest --test-dir build-macos-dev --output-on-failure
./packaging/build_macos.sh
./packaging/create_dmg.sh
```

The isolated smoke check tests Qt widgets, ZIP creation/readback, SQLite, required GStreamer elements,
SoundTouch/EQ audio processing, and synthetic CDG decoding. It never opens the
real song library or settings. It does not replace a show test with audio output,
external displays, pitch/tempo changes, break music, recording and real media.

The standalone app is `dist/OpenKJ.app`; the DMG is in `releases/`.
`SOUNDTOUCH_PLUGIN=/absolute/path/libgstsoundtouch.dylib` remains available for
an alternate plugin build. Existing developer setups keep their original plugin
fallback until the portable setup is run. The plugin is still SoundTouch; the
ten-band EQ still uses GStreamer's equalizer element.

Dependency notices, installed Homebrew formula metadata/receipts, static-library
licenses and the build environment are included under
`Contents/Resources/third-party` before signing. Qt license texts are downloaded
from the exact installed release and cached under `build-macos-deps/license-cache`
for subsequent builds. The inventory also includes build-only Homebrew formulae.

## GitHub builds and prereleases

For releases covering all platforms, use [coordinated prereleases](RELEASING.md).
The platform-specific tag path below remains available for individual previews.


`macos-build.yml` uses a GitHub-hosted `macos-15` Apple Silicon runner, the same
setup script, runtime checks, standalone packaging, ad-hoc signing and DMG
verification. Branch pushes and pull requests targeting `master` produce
downloadable build artifacts without publishing a release.

To publish a future prerelease:

1. Update all version defines in `src/okjversion.h` and
   `packaging/RELEASE_NOTES.md`, then commit and push those changes.
2. Push an annotated tag such as `macos-v2.2.4-prerelease.1` pointing to that
   commit. `macos-vX.Y.Z-preview.N` is also supported.
3. The workflow checks tag/source/notes versions, builds and verifies the DMG,
   creates a draft prerelease, uploads the DMG and SHA-256 file, then publishes it.

The workflow uses the repository's `GITHUB_TOKEN`; no personal access token,
local paths, developer signing credentials, or self-hosted runner is required.
Existing releases can receive a missing asset; existing assets are never
overwritten automatically. A failed new upload leaves a draft for inspection.
The obsolete Qt 5 and legacy Windows release/version-update workflows have been retired.
Manual workflow runs use the selected branch or tag; selecting a release tag
enables the same publishing path. Branch runs do not publish.

Builds remain experimental and ad-hoc signed, without Developer ID signing or
Apple notarization. This workflow does not create a stable release or change
the existing 2.2.3 asset. Minimum supported macOS still needs validation on real
machines; Homebrew's bottled dependencies can impose a newer minimum.

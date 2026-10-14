#!/usr/bin/env bash
# Fedora development setup; no changes to audio processing or macOS packaging.
set -euo pipefail
cd "$(dirname "$0")/.."
source /etc/os-release
[[ ${ID:-} == fedora ]] || { echo 'This setup targets Fedora Linux.' >&2; exit 1; }
if [[ ${1:-} == --install-deps ]]; then
    root_command=()
    if (( EUID != 0 )); then root_command=(sudo); fi
    "${root_command[@]}" dnf install -y \
        gcc gcc-c++ cmake ninja-build pkgconf-pkg-config git curl cargo rust \
        qt6-qtbase-devel qt6-qtbase-sqlite qt6-qtsvg-devel qt6-qtmultimedia-devel \
        qt6-qtnetworkauth-devel qt6-qtwebsockets-devel qt6-qtwayland \
        gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-plugins-good \
        gstreamer1-plugins-bad-free gstreamer1-plugins-bad-free-extras \
        gstreamer1-plugin-libav
elif [[ $# != 0 ]]; then
    echo 'Usage: packaging/setup_fedora.sh [--install-deps]' >&2
    exit 1
fi

deps="$PWD/build-linux-deps"
mkdir -p "$deps/plugins"
export GST_PLUGIN_PATH_1_0="$deps/plugins${GST_PLUGIN_PATH_1_0:+:$GST_PLUGIN_PATH_1_0}"

# GStreamer 1.28 moved CDG to gst-plugins-rs. Prefer a working installed plugin;
# otherwise build just CDG at the same upstream version tested on Windows.
if ! gst-inspect-1.0 cdgparse >/dev/null 2>&1 || ! gst-inspect-1.0 cdgdec >/dev/null 2>&1; then
    revision=6302bea23b53e6461c104f9543df887a63b2d6ec # gst-plugins-rs 0.15.3
    source_dir="$deps/gst-plugins-rs"
    if [[ ! -d "$source_dir/.git" ]]; then
        git init "$source_dir"
        git -C "$source_dir" remote add origin https://github.com/GStreamer/gst-plugins-rs.git
        git -C "$source_dir" fetch --depth 1 origin "$revision"
        git -C "$source_dir" checkout --detach FETCH_HEAD
    fi
    [[ $(git -C "$source_dir" rev-parse HEAD) == "$revision" ]] || {
        echo "Unexpected CDG source revision in $source_dir; use a fresh dependency directory." >&2
        exit 1
    }
    cargo build --manifest-path "$source_dir/Cargo.toml" --locked --release -p gst-plugin-cdg
    cp "$source_dir/target/release/libgstcdg.so" "$deps/plugins/"
fi

for element in pitch equalizer-10bands cdgparse cdgdec avdec_h264 avdec_aac; do
    if ! gst-inspect-1.0 "$element" > "$deps/$element.txt"; then
        echo "Missing media element: $element. See packaging/LINUX_DEVELOPMENT.md for Fedora codec setup." >&2
        exit 1
    fi
done

printf 'export GST_PLUGIN_PATH_1_0=%q\n' "$GST_PLUGIN_PATH_1_0" > "$deps/env.sh"
rpm -qa | sort > "$deps/environment.txt"
cmake -S . -B build-linux-dev -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOPENKJ_BUILD_SMOKE_TESTS=ON
echo 'Configured. Next: source build-linux-deps/env.sh; cmake --build build-linux-dev --parallel 4'

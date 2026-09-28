{
  lib,
  stdenv,
  src,
  cmake,
  ninja,
  pkg-config,
  wrapQtAppsHook,
  qtbase,
  qtsvg,
  qtmultimedia,
  taglib_1,
  spdlog,
  fmt,
  gst_all_1,
}:

let
  # Runtime plugin set: CDG/MP3 handling needs base+good, MP4/AAC needs bad+libav.
  gstPlugins = with gst_all_1; [
    gst-plugins-base
    gst-plugins-good
    gst-plugins-bad
    gst-plugins-ugly
    gst-libav
  ];

  version =
    let
      header = builtins.readFile "${src}/src/okjversion.h";
      match = builtins.match ".*OKJ_VERSION_STRING \"([0-9.]+)\".*" header;
    in
    if match == null then "0" else builtins.head match;
in

stdenv.mkDerivation {
  pname = "openkj";
  inherit version src;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    wrapQtAppsHook
  ];

  buildInputs = [
    qtbase
    qtsvg
    qtmultimedia
    # The sources use the TagLib 1.x C++ API; taglib 2.x moved headers and
    # changed FileRef's signatures, so pinning to 1.x is deliberate.
    taglib_1
    spdlog
    fmt
    gst_all_1.gstreamer
  ]
  ++ gstPlugins;

  # Deliberately no SPDLOG_USE_CPM / SPDLOG_USE_BUNDLED flags: CMakeLists.txt tests
  # them with `if (NOT DEFINED ...)`, so passing either one — even set to OFF —
  # disables system-spdlog detection and falls through to the CPM network fetch,
  # which cannot work in the sandbox. Leaving them unset selects the pkg-config path.

  # GStreamer resolves plugins at runtime, so the wrapper needs the plugin path
  # baked in or playback fails with "no decoder available".
  qtWrapperArgs = [
    "--prefix GST_PLUGIN_SYSTEM_PATH_1_0 : ${lib.makeSearchPathOutput "lib" "lib/gstreamer-1.0" (gstPlugins ++ [ gst_all_1.gstreamer ])}"
  ];

  meta = {
    description = "Open source karaoke show hosting software";
    homepage = "https://openkj.org";
    license = lib.licenses.gpl3Only;
    mainProgram = "openkj";
    platforms = lib.platforms.linux;
  };
}

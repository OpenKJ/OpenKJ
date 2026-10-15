#!/usr/bin/env python3
"""Stage the tested UCRT64 runtime; run with native UCRT64 Python on Windows."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def output(*args):
    return subprocess.check_output(args, text=True, encoding="utf-8", errors="replace")


def stage(build, prefix, destination):
    if os.name != "nt":
        raise RuntimeError("Run this script in Windows MSYS2 UCRT64 using its native Python")
    if destination.exists():
        raise RuntimeError(f"Use a fresh staging directory: {destination}")
    if not (build / "openkj.exe").is_file():
        raise RuntimeError(f"Build openkj.exe first: {build}")
    destination.mkdir(parents=True)
    origins = {}

    def copy(source, relative):
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        origins[target.relative_to(destination).as_posix()] = str(source)

    # Use exactly the Qt deployment tree produced by windeployqt, excluding
    # CMake's build/dependency trees. Include the offscreen platform for checks.
    for source in build.glob("*.dll"):
        copy(source, source.name)
    for name in ("openkj.exe", "openkj-runtime-smoke.exe"):
        copy(build / name, name)
    qt_plugins = Path(output(str(prefix / "bin/qmake6.exe"), "-query", "QT_INSTALL_PLUGINS").strip())
    for directory in qt_plugins.iterdir():
        deployed = build / directory.name
        if directory.is_dir() and deployed.is_dir():
            for source in deployed.rglob("*.dll"):
                # windeployqt copies optional SQL drivers whose client SDKs
                # need not be installed. OpenKJ uses SQLite exclusively.
                if directory.name == "sqldrivers" and source.name.lower() != "qsqlite.dll":
                    continue
                copy(source, source.relative_to(build))
    copy(qt_plugins / "platforms/qoffscreen.dll", "platforms/qoffscreen.dll")
    (destination / "qt.conf").write_text("[Paths]\nPrefix=.\nPlugins=.\n", encoding="utf-8")

    # Preserve the installed plugin set rather than silently dropping support
    # for a format that worked in the development build. Only runtime DLLs.
    plugins = list((prefix / "lib/gstreamer-1.0").glob("*.dll"))
    if not plugins:
        raise RuntimeError("No GStreamer plugins in the selected UCRT64 prefix")
    for source in plugins:
        copy(source, Path("gstreamer-1.0") / source.name)
    copy(prefix / "libexec/gstreamer-1.0/gst-plugin-scanner.exe", "gst-plugin-scanner.exe")

    # Follow PE imports recursively for the app, Qt/GStreamer plugins, scanner,
    # FFmpeg, SoundTouch, compiler runtime and their transitive dependencies.
    # Never resolve against another SDK or ship DLLs from Windows itself.
    binaries = {p.name.lower(): p for p in (prefix / "bin").glob("*.dll")}
    system = Path(os.environ["SystemRoot"]) / "System32"
    system_names = {p.name.lower() for p in system.glob("*.dll")}
    queue = list(destination.rglob("*.dll")) + list(destination.glob("*.exe"))
    visited = set()
    while queue:
        binary = queue.pop()
        if binary in visited:
            continue
        visited.add(binary)
        imports = re.findall(r"DLL Name:\s*(\S+)", output(str(prefix / "bin/objdump.exe"), "-p", str(binary)))
        for name in imports:
            key = name.lower()
            if key == "msys-2.0.dll":
                raise RuntimeError(f"MSYS binary found in native runtime: {binary}")
            if key.startswith(("api-ms-", "ext-ms-")):
                continue
            if key in binaries:
                target = destination / binaries[key].name
                if not target.exists():
                    copy(binaries[key], target.name)
                    queue.append(target)
            elif key not in system_names:
                raise RuntimeError(f"Unresolved dependency {name} imported by {binary}")

    # Retain SDK notices and record exact package versions/build recipes. Static
    # CPM libraries' notices are separate from the dynamically linked runtime.
    shutil.copytree(prefix / "share/licenses", destination / "third-party/licenses")
    for dependency, names in {
        "libzip": ["LICENSE"], "taglib": ["COPYING.LGPL", "COPYING.MPL"],
        "spdlog": ["LICENSE", "include/spdlog/fmt/bundled/LICENSE.rst"],
    }.items():
        for name in names:
            copy(build / f"_deps/{dependency}-src" / name, Path("third-party") / dependency / name)
    root = Path(__file__).resolve().parents[1]
    copy(root / "LICENSE", "LICENSE.txt")
    (destination / "third-party/packages.txt").write_text(output("pacman", "-Q"), encoding="utf-8")
    (destination / "third-party/README.txt").write_text(
        "Runtime binaries come from MSYS2 UCRT64 packages listed in packages.txt.\n"
        "License texts are in licenses/; this inventory includes installed SDK packages.\n"
        "Package source/build recipes: https://github.com/msys2/MINGW-packages\n"
        "Package archive: https://repo.msys2.org/mingw/ucrt64/\n"
        "OpenKJ source, including static-library version pins and build scripts:\n"
        "https://github.com/EquinoxEarth/OpenKJ/tree/" + output("git", "rev-parse", "HEAD").strip() + "\n"
        "Qt, GStreamer, FFmpeg and SoundTouch remain separate dynamic libraries.\n",
        encoding="utf-8")
    (destination / "README.txt").write_text(
        "OpenKJ Windows x64 experimental preview (unsigned)\n\n"
        "Extract the entire ZIP into a writable folder, then open openkj.exe.\n"
        "Keep all DLLs and plugin folders together. MSYS2 is not required.\n"
        "Use your normal Windows user account; administrator rights are not needed.\n"
        "This uses your normal OpenKJ settings/library; back up existing data first.\n"
        "third-party/ contains dependency versions and license notices.\n"
        "openkj-runtime-smoke.exe is an isolated runtime diagnostic, not the app.\n",
        encoding="utf-8")
    manifest = {
        "commit": output("git", "rev-parse", "HEAD").strip(),
        "files": {p.relative_to(destination).as_posix(): {
            "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
            "origin": origins.get(p.relative_to(destination).as_posix(), "generated or SDK license"),
        } for p in sorted(destination.rglob("*")) if p.is_file()},
    }
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Staged {len(visited)} native binaries and {len(plugins)} GStreamer plugins in {destination}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build-windows-dev"))
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("dist/OpenKJ-Windows-x64-preview"))
    arguments = parser.parse_args()
    try:
        stage(arguments.build_dir.resolve(), arguments.prefix.resolve(), arguments.output.resolve())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))

#!/usr/bin/env python3
"""Collect installed dependency notices before the macOS bundle is signed."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import urllib.request


def collect(app, build, dependencies):
    root = Path(__file__).resolve().parents[1]
    destination = app / 'Contents/Resources/third-party'
    if destination.exists():
        raise RuntimeError(f'Use a freshly staged bundle: {destination}')
    destination.mkdir(parents=True)
    shutil.copyfile(root / 'LICENSE', destination / 'OpenKJ-LICENSE.txt')
    metadata = json.loads(subprocess.check_output(['brew', 'info', '--json=v2', '--installed'], text=True))
    cellar = Path(subprocess.check_output(['brew', '--cellar'], text=True).strip())
    # Like the Windows SDK notice collection, retain notices for all installed
    # formulae; the inventory can include build tools and unbundled components.
    # Keep original relative paths so distinct notices never overwrite each other.
    for formula in metadata['formulae']:
        for installed in formula['installed']:
            prefix = cellar / formula['name'] / installed['version']
            if not prefix.is_dir():
                raise RuntimeError(f'Missing installed formula: {prefix}')
            package = destination / 'homebrew' / formula['name'] / installed['version']
            package.mkdir(parents=True)
            for source in prefix.rglob('*'):
                legal_name = re.match(r'^(licen[cs]e|copying|copyright|notice)([._-]|$)', source.name, re.I)
                legal_folder = any(part.lower() in ('licenses', 'licences') for part in source.relative_to(prefix).parts[:-1])
                if source.is_file() and not source.is_symlink() and (legal_name or legal_folder):
                    target = package / source.relative_to(prefix)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, target)
            shutil.copyfile(prefix / 'INSTALL_RECEIPT.json', package / 'INSTALL_RECEIPT.json')
    (destination / 'homebrew-formulae.json').write_text(json.dumps(metadata, indent=2) + '\n')
    qt_prefix = Path(subprocess.check_output(['brew', '--prefix', 'qtbase'], text=True).strip()).resolve()
    qt_version = re.match(r'\d+\.\d+\.\d+', qt_prefix.name)[0]
    # Homebrew Qt does not install its top-level license texts. Obtain them
    # from the exact installed Qt release, not a moving development branch.
    cache = dependencies / 'license-cache' / ('qt-' + qt_version)
    cache.mkdir(parents=True, exist_ok=True)
    sources = {}
    for name in ('LGPL-3.0-only.txt', 'GPL-3.0-only.txt', 'GPL-2.0-only.txt', 'Qt-GPL-exception-1.0.txt'):
        url = f'https://raw.githubusercontent.com/qt/qtbase/v{qt_version}/LICENSES/{name}'
        cached = cache / name
        if not cached.exists():
            with urllib.request.urlopen(url, timeout=60) as response:
                cached.write_bytes(response.read())
        data = cached.read_bytes()
        target = destination / 'qt' / name
        target.parent.mkdir(exist_ok=True)
        target.write_bytes(data)
        sources[name] = url
    (destination / 'qt/source-urls.json').write_text(json.dumps(sources, indent=2) + '\n')
    for dependency, names in {
        'libzip': ['LICENSE'], 'taglib': ['COPYING.LGPL', 'COPYING.MPL'],
        'spdlog': ['LICENSE', 'include/spdlog/fmt/bundled/LICENSE.rst'],
    }.items():
        for name in names:
            source = build / f'_deps/{dependency}-src' / name
            target = destination / 'static' / dependency / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
    plugin_license = dependencies / 'gst-plugins-bad-1.28.8/COPYING'
    if not plugin_license.exists():
        # Keep the original external-plugin build path usable as well.
        plugin_license = dependencies / 'license-cache/gstreamer-soundtouch-COPYING'
        plugin_license.parent.mkdir(parents=True, exist_ok=True)
        if not plugin_license.exists():
            url = 'https://raw.githubusercontent.com/GStreamer/gstreamer/1.28.8/subprojects/gst-plugins-bad/COPYING'
            with urllib.request.urlopen(url, timeout=60) as response:
                plugin_license.write_bytes(response.read())
    shutil.copyfile(plugin_license, destination / 'gstreamer-soundtouch-COPYING')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    (destination / 'README.txt').write_text(
        'OpenKJ and bundled dependencies retain their respective licenses.\n'
        'Homebrew notices/receipts cover installed formulae, including build-only tools.\n'
        'homebrew-formulae.json records installed versions, license identifiers and source URLs.\n'
        'Homebrew build recipes: https://github.com/Homebrew/homebrew-core\n'
        'Qt license texts are from the installed release; URLs are in qt/source-urls.json.\n'
        'Static dependency versions and the custom SoundTouch plugin recipe are in OpenKJ source:\n'
        f'https://github.com/EquinoxEarth/OpenKJ/tree/{commit}\n'
        'Build environment: see build-environment.txt.\n')
    environment = dependencies / 'environment.txt'
    if environment.exists():
        shutil.copyfile(environment, destination / 'build-environment.txt')
    else:
        (destination / 'build-environment.txt').write_text(subprocess.check_output(['brew', 'list', '--versions'], text=True))
    print(f'Collected dependency notices in {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app', type=Path)
    parser.add_argument('--build', type=Path, default=Path('build-macos-dev'))
    parser.add_argument('--dependencies', type=Path, default=Path('build-macos-deps'))
    args = parser.parse_args()
    collect(args.app, args.build, args.dependencies)

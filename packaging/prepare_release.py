#!/usr/bin/env python3
"""Verify platform artifacts from one CI run and stage a coordinated prerelease."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import zipfile


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def prepare(artifacts, destination, tag, commit):
    match = re.fullmatch(r'v(\d+\.\d+\.\d+)-(?:prerelease|preview)\.\d+', tag)
    if not match or not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('Expected a prerelease tag and full source commit SHA')
    version = match[1]
    root = Path(__file__).resolve().parents[1]
    header = (root / 'src/okjversion.h').read_text()
    if re.search(r'OKJ_VERSION_STRING "([^"\n]+)"', header)[1] != version:
        raise ValueError('Release tag does not match source version')
    windows = artifacts / 'windows-zip/OpenKJ-Windows-x64-preview.zip'
    with zipfile.ZipFile(windows) as archive:
        manifest = json.loads(archive.read('OpenKJ-Windows-x64-preview/manifest.json'))
        if manifest['commit'] != commit:
            raise ValueError('Windows source commit does not match release')
    linux = artifacts / 'linux'
    manifest = json.loads((linux / 'build-manifest.json').read_text())
    if manifest['openkj-source-commit'] != commit:
        raise ValueError('Linux source commit does not match release')
    macos = artifacts / 'macos'
    if '/commit/' + commit not in (macos / 'RELEASE_NOTES.md').read_text():
        raise ValueError('macOS source commit does not match release')
    files = [
        (macos / f'OpenKJ-{version}-macos-arm64.dmg', f'OpenKJ-{version}-macos-arm64.dmg'),
        (windows, f'OpenKJ-{version}-windows-x64.zip'),
        (artifacts / f'windows-msi/OpenKJ-{version}-windows-x64.msi', f'OpenKJ-{version}-windows-x64.msi'),
        (linux / 'OpenKJ-Linux-x86_64-preview.flatpak', f'OpenKJ-{version}-linux-x86_64.flatpak'),
    ]
    # Validate all inputs before creating the output directory or copying files.
    for source, _ in files:
        fields = Path(str(source) + '.sha256').read_text(encoding='utf-8-sig').split()
        if len(fields) != 2 or fields[1] != source.name or fields[0].lower() != digest(source):
            raise ValueError(f'Checksum mismatch: {source}')
    notes = (root / 'packaging/RELEASE_NOTES.md').read_text()
    if not notes.startswith('# OpenKJ ' + version + ' —'):
        raise ValueError('Release notes do not match version')
    destination.mkdir(parents=True, exist_ok=False)
    for source, name in files:
        target = destination / name
        shutil.copyfile(source, target)
        Path(str(target) + '.sha256').write_text(f'{digest(target)}  {name}\n')
    shutil.copyfile(linux / 'build-manifest.json', destination / f'OpenKJ-{version}-linux-build-manifest.json')
    shutil.copyfile(linux / 'runtime-versions.txt', destination / f'OpenKJ-{version}-linux-runtime-versions.txt')
    info = {'version': version, 'tag': tag, 'source_commit': commit,
            'source_url': f'https://github.com/EquinoxEarth/OpenKJ/commit/{commit}',
            'packages': {name: digest(destination / name) for _, name in files}}
    (destination / f'OpenKJ-{version}-build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    notes += f'\nSource: [release commit]({info["source_url"]}).\n'
    (destination / 'RELEASE_NOTES.md').write_text(notes)
    checksums = ''.join(f'{digest(p)}  {p.name}\n' for p in sorted(destination.iterdir()) if p.suffix != '.sha256')
    (destination / 'SHA256SUMS').write_text(checksums)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--commit', required=True)
    args = parser.parse_args()
    prepare(args.artifacts, args.output, args.tag, args.commit)

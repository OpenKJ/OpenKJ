#!/usr/bin/env python3
"""Collect Homebrew dylib dependencies and make them bundle-relative.

Usage: python3 packaging/relocate_macos.py dist/OpenKJ.app
This modifies the bundle and invalidates signatures. Sign afterward.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path


def run(*args, check=True):
    p = subprocess.run(args, text=True, capture_output=True)
    if check and p.returncode:
        raise RuntimeError(f"Command failed: {' '.join(map(str, args))}\n{p.stderr}")
    return p.stdout


def macho(path):
    return "Mach-O" in run("file", "-b", str(path), check=False)


def otool_refs(path):
    return [line.strip().split(" (")[0] for line in run("otool", "-L", str(path)).splitlines()[1:]]


def dylib_id(path):
    lines = run("otool", "-D", str(path), check=False).splitlines()
    return lines[1].strip() if len(lines) > 1 else None


def external(dep):
    return dep.startswith(("/opt/homebrew/", "/usr/local/"))


def main():
    if len(sys.argv) != 2:
        sys.exit("Usage: relocate_macos.py path/to/OpenKJ.app")
    app = Path(sys.argv[1]).expanduser().resolve()
    contents = app / "Contents"
    if not (contents / "MacOS" / "openkj").is_file():
        sys.exit(f"Missing app executable: {app}")
    fw = contents / "Frameworks"
    fw.mkdir(parents=True, exist_ok=True)
    seen = set()
    n_copy = n_change = 0

    def binaries():
        for p in contents.rglob("*"):
            if p.is_file() and not p.is_symlink() and macho(p):
                yield p

    def bundled(name):
        # Third-party dylibs live at top level of Contents/Frameworks.
        candidate = fw / name
        return candidate if candidate.is_file() else None

    def resolve_external(dep):
        nonlocal n_copy
        source = Path(dep)
        if not source.is_file():
            raise RuntimeError(f"Missing Homebrew dependency: {dep}")
        dest = fw / source.name
        if not dest.exists():
            print(f"COPY {source.name}")
            shutil.copy2(source, dest)
            n_copy += 1
        return dest

    while True:
        pending = [p for p in binaries() if p not in seen]
        if not pending:
            break
        for binary in pending:
            seen.add(binary)
            own_id = dylib_id(binary)
            for dep in otool_refs(binary):
                if dep == own_id:
                    continue
                target = None
                if external(dep):
                    if ".framework/" in dep:
                        raise RuntimeError(f"Unbundled external framework in {binary}: {dep}")
                    target = resolve_external(dep)
                elif dep.startswith("@rpath/") and dep.endswith(".dylib"):
                    # Only rewrite known bundled third-party dylibs.
                    target = bundled(Path(dep).name)
                if target is None:
                    continue
                rel = os.path.relpath(target, binary.parent)
                new_ref = f"@loader_path/{rel}"
                if dep != new_ref:
                    run("install_name_tool", "-change", dep, new_ref, str(binary))
                    n_change += 1
                    print(f"FIX {binary.name}: {Path(dep).name} -> {new_ref}")

    # Perform a full audit of *load* dependencies (not install IDs).
    missing = []
    for binary in binaries():
        own_id = dylib_id(binary)
        for dep in otool_refs(binary):
            if dep == own_id:
                continue
            if external(dep):
                missing.append(f"{binary}: external dependency {dep}")
            elif dep.startswith("@loader_path/"):
                target = (binary.parent / dep.removeprefix("@loader_path/")).resolve()
                if not target.is_file():
                    missing.append(f"{binary}: missing bundled dependency {dep}")
            elif dep.startswith("@rpath/") and dep.endswith(".dylib"):
                if not bundled(Path(dep).name):
                    missing.append(f"{binary}: unresolved third-party rpath {dep}")
    if missing:
        print("\nDependency audit failed:", file=sys.stderr)
        print("\n".join(missing), file=sys.stderr)
        sys.exit(1)
    print(f"PASS: relocated {n_change} references; copied {n_copy} libraries; no external Homebrew load dependencies")


if __name__ == "__main__":
    main()

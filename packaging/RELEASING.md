# Coordinated prereleases

The main release path is `.github/workflows/release.yml`. An annotated tag such
as `v2.2.4-prerelease.1` builds macOS ARM64, Windows x64 ZIP/MSI, and Linux x86_64
Flatpak from the same commit. Packages remain experimental prereleases.

1. Update `src/okjversion.h`, both release-note files, and the first release in
   `src/org.openkj.OpenKJ.metainfo.xml`. Keep numeric versions consistent.
2. Commit the changes on `master`. Run the Cross-platform prerelease workflow
   manually on that commit for a complete rehearsal without publishing.
3. Review artifacts and check the Windows MSI wizard on a real Windows PC.
4. Push an annotated tag `vX.Y.Z-prerelease.N` pointing to the reviewed commit.
5. The reusable build workflows test their packages. Windows also tests install,
   upgrade, repair, installed runtime, downgrade rejection and uninstall. Flatpak
   also tests clean Ubuntu/Fedora installations without a development SDK.
6. Only after all jobs pass does the publisher verify source provenance and
   checksums, create a draft, attach all packages/metadata, and publish it as a
   prerelease. It never overwrites an existing release or existing assets.

Each package has a SHA-256 file; `SHA256SUMS` also covers release notes and build
metadata. ZIP/Flatpak names are versioned during release staging, after checking
their original CI checksums. The portable ZIP, installed MSI, DMG and Flatpak
are the distribution assets. Development builds and CI diagnostics stay in
Actions artifacts rather than the public release.

The older `macos-v...` and `windows-v...` tag paths remain available for platform
previews; they are not needed for a coordinated release. Selecting the new
workflow manually builds/tests all platforms and does not create a release.
If upload fails after draft creation, inspect the draft and failed run before
retrying; publishing refuses to overwrite that draft automatically. Never move
an already published tag to a different commit.

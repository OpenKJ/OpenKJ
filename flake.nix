{
  description = "OpenKJ — open source karaoke show hosting software";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
      openkjFor = pkgs: pkgs.libsForQt5.callPackage ./nix/openkj.nix { src = self; };
    in
    {
      packages = forAllSystems (pkgs: rec {
        openkj = openkjFor pkgs;
        default = openkj;
      });

      overlays.default = final: _prev: { openkj = openkjFor final; };

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ (openkjFor pkgs) ];
          # clang-tools for clangd and clang-tidy against the compile database.
          packages = [ pkgs.clang-tools ];
        };
      });
    };
}

{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0"; # stable Nixpkgs

  outputs =
    { self, ... }@inputs:

    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f:
        inputs.nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            inherit system;
            pkgs = import inputs.nixpkgs { inherit system; };
          }
        );
    in
    {
      devShells = forEachSupportedSystem (
        { pkgs, system }: {
          default =
            pkgs.mkShell.override
              {
                # Override stdenv in order to change compiler:
                # stdenv = pkgs.clangStdenv;
              }
              {
                packages =
                  with pkgs;
                  [
                    clang-tools
                    cmake
                    ninja
                    self.formatter.${system}
                    gnumake
                    gcc
                    pkg-config
                  ]
                  ++ lib.optionals (!stdenv.hostPlatform.isDarwin) [ gdb ];
                buildInputs =
                  with pkgs;
                  [
                    qt6.qtbase
                    qt6.qtmultimedia
                  ]
                  ++ lib.optionals stdenv.hostPlatform.isLinux [
                    qt6.qtwayland
                    libGL
                    libxkbcommon
                  ];
                CMAKE_GENERATOR = "Ninja";
              };
        }
      );

      formatter = forEachSupportedSystem ({ pkgs, ... }: pkgs.nixfmt);
    };
}

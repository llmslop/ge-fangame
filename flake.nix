{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default";
    flake-utils.url = "github:numtide/flake-utils";
    git-hooks.url = "github:cachix/git-hooks.nix";
    jailed-agents.url = "github:btmxh/jailed-agents";
    jail-nix.url = "sourcehut:~alexdavid/jail.nix";
  };

  outputs =
    {
      self,
      nixpkgs,
      systems,
      git-hooks,
      jailed-agents,
      jail-nix,
      ...
    }:
    let
      forEachSystem = nixpkgs.lib.genAttrs (import systems);
    in
    {
      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          python = pkgs.python3.withPackages (
            ps: with ps; [
              pillow
              types-pillow
              soundfile
            ]
          );
          inherit (self.checks.${system}.pre-commit-check) shellHook enabledPackages config;
          inherit (config) package configFile;
          jail = jail-nix.lib.init pkgs;
        in
        {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config

              # this order matters: we want to have the host gcc as the default option
              gcc-arm-embedded
              gcc

              python
            ];

            buildInputs =
              with pkgs;
              [
                sdl3
              ]
              ++ enabledPackages;

            shellHook = shellHook + ''
              export CMAKE_EXPORT_COMPILE_COMMANDS=1
              export CMAKE_GENERATOR="Ninja Multi-Config"
              export CMAKE_COLOR_DIAGNOSTICS=ON
            '';

            packages =
              with pkgs;
              [
                openocd
                gdb
                tio
                ffmpeg

                # tooling
                clang-tools
                nixd
                nixfmt-rfc-style
                ty

                # cmake
                gersemi
                cmake-lint
                neocmakelsp
              ]
              ++ (builtins.attrValues (
                jailed-agents.lib.${system}.makeJailedAgents {
                  extraPkgs = [
                    ruff
                    ty
                    nixfmt-rfc-style
                    package
                    cmake
                    ninja
                    pkg-config
                    gcc-arm-embedded
                    gcc
                    clang-tools
                    gersemi
                    sdl3
                    python
                  ]
                  ++ enabledPackages;

                  extraJailOptions = with jail.combinators; [
                    (readonly configFile)
                    (readonly (lib.getExe package))
                    (readonly (lib.getDev sdl3))
                    (readonly (lib.getLib sdl3))
                    (fwd-env "CMAKE_EXPORT_COMPILE_COMMANDS")
                    (fwd-env "CMAKE_GENERATOR")
                    (fwd-env "CMAKE_COLOR_DIAGNOSTICS")
                    (fwd-env "PKG_CONFIG")
                    (fwd-env "CMAKE_LIBRARY_PATH")
                    (fwd-env "NIXPKGS_CMAKE_PREFIX_PATH")
                  ];
                }
              ));
          };
        }
      );

      checks = forEachSystem (system: {
        pre-commit-check = git-hooks.lib.${system}.run {
          src = ./.;
          hooks = {
            nixfmt.enable = true;
            statix.enable = true;
            check-yaml.enable = true;
            end-of-file-fixer = {
              enable = true;
              excludes = [ ".*\\.bin" ];
            };
            clang-format.enable = true;
            trim-trailing-whitespace.enable = true;
            ruff.enable = true;
            ruff-format.enable = true;
          };
        };
      });
    };
}

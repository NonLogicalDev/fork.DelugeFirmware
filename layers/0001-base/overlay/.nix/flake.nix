{
  description = "Local Deluge firmware build environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      system = "aarch64-darwin";
      pkgs = import nixpkgs { inherit system; };
      toolchainVersion = "22";
      toolchain = builtins.fetchTarball {
        url = "https://github.com/SynthstromAudible/dbt-toolchain/releases/download/v${toolchainVersion}/dbt-toolchain-${toolchainVersion}-darwin-arm64.tar.gz";
        sha256 = "sha256-EXI2bX2mGJl14ZXtVfUaa7xRenWxfHg0PLPBsWJSOtc=";
      };
      toolchainLayout = pkgs.runCommand "deluge-toolchain-layout" { } ''
        mkdir -p "$out/toolchain/v${toolchainVersion}"
        ln -s ${toolchain} "$out/toolchain/v${toolchainVersion}/darwin-arm64"
      '';
      python = pkgs.python3.withPackages (ps: [
        ps.ansi
        ps.certifi
        ps.pyserial
        ps.setuptools
      ]);
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          pkgs.git
          pkgs.pre-commit
          pkgs.ccache
          python
        ];

        DBT_TOOLCHAIN_PATH = "${toolchainLayout}";

        shellHook = ''
          toolchain="${toolchain}"
          export PATH="$toolchain/arm-none-eabi-gcc/bin:$toolchain/cmake/bin:$toolchain/ninja-build/bin:$toolchain/clang-format:$toolchain/openocd/bin:$PATH"
          export DELUGE_FW_ROOT="$PWD"
          export CCACHE_DIR="$DELUGE_FW_ROOT/.cache/ccache"
          export CCACHE_BASEDIR="$DELUGE_FW_ROOT"
          export CCACHE_COMPILERCHECK=content
          export CMAKE_C_COMPILER_LAUNCHER=ccache
          export CMAKE_CXX_COMPILER_LAUNCHER=ccache
          export DBT_NO_PYTHON_UPGRADE=1
          export PYTHONNOUSERSITE=1
          export PYTHONPATH="$DELUGE_FW_ROOT/scripts${PYTHONPATH:+:$PYTHONPATH}"
        '';
      };
    };
}

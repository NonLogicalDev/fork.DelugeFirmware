# Deluge build environment

From the repository root, enter the local environment with `nix develop path:./.nix`.

It pins the upstream v22 macOS ARM toolchain and exposes it through `DBT_TOOLCHAIN_PATH`, along with Git, pre-commit, the Python helpers, and `ccache`. The compiler cache is stored at the ignored checkout-local path `.cache/ccache`; it never changes the selected compiler or the firmware output.

Build a local Release binary with `DBT_NO_SYNC=1 ./dbt build Release`. The output is `build/Release/deluge.bin`. This does not flash or publish firmware.

For a build directory created before `ccache` was added, reconfigure it once inside the environment with `DBT_NO_SYNC=1 ./dbt configure`. Later normal builds use the configured compiler launchers. Run `ccache --show-stats` to inspect cache use.

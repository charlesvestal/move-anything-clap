# Move Anything CLAP Host Module

A Move Anything module that hosts arbitrary CLAP plugins in-process, usable as a standalone sound generator or as a component in Signal Chain.

## Features

- Load any CLAP plugin from `/data/UserData/move-anything/modules/clap/plugins/`
- Browse and select plugins via the QuickJS UI
- Control plugin parameters via encoder banks
- Usable as sound generator in Signal Chain patches
- CLAP audio FX plugins can be used in the chain's audio FX slot

## Installation

### From Release

```bash
./scripts/install.sh
```

### From Source

```bash
./scripts/build.sh
./scripts/install.sh
```

## Usage

1. Copy `.clap` plugin files to `/data/UserData/move-anything/modules/clap/plugins/` on the Move
2. Select the CLAP module from the host menu
3. Use the UI to browse and load plugins
4. Adjust parameters with the encoders

## Building

Requires the aarch64-linux-gnu cross-compiler or Docker.

```bash
./scripts/build.sh
```

## License

MIT License - see LICENSE file

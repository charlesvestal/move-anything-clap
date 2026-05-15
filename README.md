# Airwindows for Schwung

Bundles 500+ Airwindows audio effects (Chris Johnson's classic free plugin
suite) as a single audio FX module for the Ableton Move via Schwung / Move
Everything.

Built on a generic CLAP host: you can also drop other ARM64 Linux `.clap`
files into the plugins directory, though Airwindows is what ships by default.

## Prerequisites

- [Schwung / Move Everything](https://github.com/charlesvestal/move-everything) installed on your Ableton Move
- SSH access enabled: http://move.local/development/ssh

## Features

- 500+ Airwindows effects (via baconpaul/airwin2rack), grouped by category
- Category-based browsing (Consoles, Filter, Tape, Reverb, etc.)
- Per-plugin parameter control via encoder banks
- Usable as an audio FX in Signal Chain
- Extensible: drop additional ARM64 Linux `.clap` files into the plugins directory

## Important: Plugin Compatibility

**The Move has specific requirements for CLAP plugins:**

- ARM64 Linux (aarch64) binaries only
- No GUI dependencies (X11, Cairo, OpenGL, GTK, etc.)
- glibc 2.35 or older (Move's system library version)
- GLIBCXX 3.4.29 or older

Most pre-built CLAP plugins are x86_64 and include GUI code, so you'll typically need to **build from source** with headless options.

### When to Use CLAP vs Native Ports

| Use CLAP Host | Consider Native Port |
|---------------|---------------------|
| Plugin builds easily headless | Complex dependency chains |
| Minimal dependencies | JUCE-based with glibc issues |
| Effects and simple synths | Major synths you'll use often |
| Quick experimentation | Need deep Move UI integration |

For complex synths like Surge XT or Vital, the effort to resolve glibc/libstdc++ compatibility issues is often comparable to porting the DSP core natively.

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

1. Copy `.clap` plugin files to `/data/UserData/move-anything/modules/sound_generators/clap/plugins/` on the Move
2. If plugins need shared libraries, copy `.so` files to the same directory
3. Select the CLAP module from the host menu
4. Use the UI to browse and load plugins
5. Adjust parameters with the encoders

## Building Plugins

See [BUILDING.md](BUILDING.md) for detailed build instructions for specific plugin frameworks (SA_Toolkit, LSP Plugins, clap-plugins, etc.).

## Bundled Plugins

### Airwindows

This module bundles [Airwindows](https://github.com/airwindows/airwindows) by Chris Johnson — 498 high-quality open source audio effects as a single CLAP plugin. The CLAP build is from [airwin2rack](https://github.com/baconpaul/airwin2rack) by Bacon Pauls.

Airwindows is licensed under the MIT License. See [LICENSES/AIRWINDOWS.txt](LICENSES/AIRWINDOWS.txt) for full license text.

## License

MIT License - see LICENSE file

## AI Assistance Disclaimer

This module is part of Move Everything and was developed with AI assistance, including Claude, Codex, and other AI assistants.

All architecture, implementation, and release decisions are reviewed by human maintainers.  
AI-assisted content may still contain errors, so please validate functionality, security, and license compatibility before production use.

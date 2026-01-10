# CLAUDE.md

Instructions for Claude Code when working with this repository.

## Project Overview

CLAP Host module for Move Anything - hosts arbitrary CLAP audio plugins in-process, usable as a sound generator or audio FX in Signal Chain.

## Build Commands

```bash
./scripts/build.sh      # Build with Docker (recommended)
./scripts/install.sh    # Deploy to Move
```

## Architecture

```
src/
  dsp/
    clap_plugin.cpp     # Main plugin (plugin_api_v1), sound generator
    clap_host.c/h       # CLAP discovery, load, process helpers
  chain_audio_fx/
    clap_fx.cpp         # Audio FX wrapper (audio_fx_api_v1)
  ui.js                 # JavaScript UI for plugin selection
  module.json           # Module metadata
  chain_patches/        # Signal Chain presets
third_party/
  clap/include/         # Vendored CLAP headers
```

## Key Implementation Details

### Plugin API

Implements Move Anything plugin_api_v1:
- `on_load`: Scans plugins directory, loads default or stored plugin
- `on_midi`: Converts MIDI to CLAP events and forwards to plugin
- `set_param`: selected_plugin, param_* for CLAP parameters, refresh
- `get_param`: plugin_count, plugin_name_*, param_count, param_name_*, param_value_*
- `render_block`: Calls CLAP process, converts float to int16

### Audio FX API

Implements audio_fx_api_v1 for Signal Chain effects:
- Same CLAP host core as main plugin
- Processes audio input → CLAP plugin → audio output

### CLAP Host Core (clap_host.c)

Shared functions:
- `clap_scan_plugins()`: Discover .clap files and read metadata
- `clap_load_plugin()`: Load plugin instance from file
- `clap_process_block()`: Run CLAP process with float buffers
- `clap_param_*()`: Parameter enumeration and control
- `clap_unload_plugin()`: Clean up plugin instance

### Plugin Discovery

Plugins scanned from `/data/UserData/move-anything/modules/clap/plugins/`.
Each `.clap` file is opened with dlopen, factory enumerated, metadata extracted.

## Signal Chain Integration

Module declares `"chainable": true` and `"component_type": "sound_generator"` in module.json.
Audio FX wrapper installed to `modules/chain/audio_fx/clap/`.
Chain presets installed to `modules/chain/patches/` by install script.

## Testing

```bash
# Header test
cc tests/test_clap_headers.c -Ithird_party/clap/include -o /tmp/test && /tmp/test

# Discovery test (requires fixtures)
cc tests/test_clap_scan.c -Isrc -Ithird_party/clap/include -ldl -o /tmp/test && /tmp/test

# Process test (requires fixtures)
cc tests/test_clap_process.c -Isrc -Ithird_party/clap/include -ldl -o /tmp/test && /tmp/test
```

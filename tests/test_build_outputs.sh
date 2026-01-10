#!/bin/sh
# Test that build produces expected outputs
set -e

echo "Testing build outputs..."

# Check dist directory exists
[ -d dist/clap ] || { echo "FAIL: dist/clap/ not found"; exit 1; }

# Check main module files
[ -f dist/clap/dsp.so ] || { echo "FAIL: dist/clap/dsp.so not found"; exit 1; }
[ -f dist/clap/module.json ] || { echo "FAIL: dist/clap/module.json not found"; exit 1; }
[ -f dist/clap/ui.js ] || { echo "FAIL: dist/clap/ui.js not found"; exit 1; }

# Check audio FX
[ -d dist/chain_audio_fx/clap ] || { echo "FAIL: dist/chain_audio_fx/clap/ not found"; exit 1; }
[ -f dist/chain_audio_fx/clap/clap.so ] || { echo "FAIL: dist/chain_audio_fx/clap/clap.so not found"; exit 1; }

echo "All build outputs present!"

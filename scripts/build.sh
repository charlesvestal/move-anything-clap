#!/usr/bin/env bash
# Build CLAP Host module for Move Anything (ARM64)
#
# Automatically uses Docker for cross-compilation if needed.
# Set CROSS_PREFIX to skip Docker (e.g., for native ARM builds).
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE_NAME="move-anything-builder"

# Check if we need Docker
if [ -z "$CROSS_PREFIX" ] && [ ! -f "/.dockerenv" ]; then
    echo "=== CLAP Host Module Build (via Docker) ==="
    echo ""

    # Build Docker image if needed
    if ! docker image inspect "$IMAGE_NAME" &>/dev/null; then
        echo "Building Docker image (first time only)..."
        docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile" "$REPO_ROOT"
        echo ""
    fi

    # Run build inside container
    echo "Running build..."
    docker run --rm \
        -v "$REPO_ROOT:/build" \
        -u "$(id -u):$(id -g)" \
        -w /build \
        "$IMAGE_NAME" \
        ./scripts/build.sh

    echo ""
    echo "=== Done ==="
    exit 0
fi

# === Actual build (runs in Docker or with cross-compiler) ===
CROSS_PREFIX="${CROSS_PREFIX:-aarch64-linux-gnu-}"

cd "$REPO_ROOT"

echo "=== Building CLAP Host Module ==="
echo "Cross prefix: $CROSS_PREFIX"

# Create build directories
mkdir -p build
mkdir -p dist/clap/plugins
mkdir -p dist/chain_audio_fx/clap

# Compile main DSP plugin (sound generator)
echo "Compiling CLAP host DSP plugin..."
${CROSS_PREFIX}g++ -O3 -shared -fPIC -std=c++14 \
    -march=armv8-a -mtune=cortex-a72 \
    -fno-exceptions \
    -DNDEBUG \
    src/dsp/clap_plugin.cpp \
    src/dsp/clap_host.c \
    -o build/dsp.so \
    -Isrc \
    -Isrc/dsp \
    -Ithird_party/clap/include \
    -ldl

# Compile audio FX plugin for Signal Chain
echo "Compiling CLAP audio FX plugin..."
${CROSS_PREFIX}g++ -O3 -shared -fPIC -std=c++14 \
    -march=armv8-a -mtune=cortex-a72 \
    -fno-exceptions \
    -DNDEBUG \
    src/chain_audio_fx/clap_fx.cpp \
    src/dsp/clap_host.c \
    -o build/clap_fx.so \
    -Isrc \
    -Isrc/dsp \
    -Isrc/chain_audio_fx \
    -Ithird_party/clap/include \
    -ldl

# Copy files to dist (use cat to avoid ExtFS deallocation issues with Docker)
echo "Packaging..."
cat src/module.json > dist/clap/module.json
cat src/ui.js > dist/clap/ui.js
cat build/dsp.so > dist/clap/dsp.so
chmod +x dist/clap/dsp.so
cat build/clap_fx.so > dist/chain_audio_fx/clap/clap.so
chmod +x dist/chain_audio_fx/clap/clap.so

# Create tarball for release
cd dist
tar -czvf clap-module.tar.gz clap/ chain_audio_fx/
cd ..

echo ""
echo "=== Build Complete ==="
echo "Output: dist/clap/ and dist/chain_audio_fx/clap/"
echo "Tarball: dist/clap-module.tar.gz"
echo ""
echo "To install on Move:"
echo "  ./scripts/install.sh"
echo ""
echo "Add your .clap plugin files to dist/clap/plugins/"

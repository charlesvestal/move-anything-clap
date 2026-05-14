#!/usr/bin/env bash
# Build plugins/Airwindows.clap from baconpaul/airwin2rack + custom wrapper.
#
# Steps:
#   1. Ensure airwin2rack is cloned (or up-to-date) under build/airwindows/airwin2rack.
#   2. Apply scripts/patches/airwin_no_skip_empty.patch so registerAirwindow keeps
#      all entries (upstream silently drops ~20 effects with empty whatText).
#   3. Build the airwin-registry static lib in the airwindows-builder Docker image.
#   4. Compile scripts/airwindows_clap_wrapper.cpp and link against the static libs.
#   5. Strip and copy result to plugins/Airwindows.clap.
#
# Re-runs are fast: cmake reuses prior build artifacts when sources haven't changed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_ROOT/build/airwindows"
AIRWIN_SRC="$BUILD_DIR/airwin2rack"
CMAKE_BUILD="$BUILD_DIR/custom-build/cmake"
CLAP_OUT_DIR="$BUILD_DIR/custom-build"
PLUGINS_DIR="$REPO_ROOT/plugins"
IMAGE_NAME="airwindows-builder"
DOCKERFILE="$SCRIPT_DIR/Dockerfile.airwindows"
AIRWIN_REPO="${AIRWIN_REPO:-https://github.com/baconpaul/airwin2rack.git}"
PATCH_FILE="$SCRIPT_DIR/patches/airwin_no_skip_empty.patch"

mkdir -p "$BUILD_DIR" "$PLUGINS_DIR"

# 1. Ensure Docker image exists.
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "Building Docker image $IMAGE_NAME (first time)..."
    docker build --platform linux/arm64 -t "$IMAGE_NAME" -f "$DOCKERFILE" "$SCRIPT_DIR"
fi

# 2. Clone or update airwin2rack.
if [ ! -d "$AIRWIN_SRC/.git" ]; then
    echo "Cloning airwin2rack..."
    git clone --recurse-submodules "$AIRWIN_REPO" "$AIRWIN_SRC"
else
    echo "Updating airwin2rack..."
    # Revert any prior patch so pull is clean.
    git -C "$AIRWIN_SRC" checkout -- src/AirwinRegistry.h 2>/dev/null || true
    git -C "$AIRWIN_SRC" pull --recurse-submodules
fi

# 3. Apply the patch (idempotent: skip if already applied).
if ! grep -q "false && r.whatText.empty()" "$AIRWIN_SRC/src/AirwinRegistry.h"; then
    echo "Applying patch: $PATCH_FILE"
    (cd "$AIRWIN_SRC" && git apply "$PATCH_FILE")
else
    echo "Patch already applied."
fi

# 4. Build airwin-registry static lib in Docker.
echo "Building airwin-registry static lib..."
docker run --rm --platform linux/arm64 \
    -v "$BUILD_DIR:/work" \
    -w /work \
    "$IMAGE_NAME" bash -c "
        set -e
        if [ ! -f custom-build/cmake/CMakeCache.txt ]; then
            cmake -S airwin2rack -B custom-build/cmake -DCMAKE_BUILD_TYPE=Release
        fi
        cmake --build custom-build/cmake --target airwin-registry -j\$(nproc)
    "

# 5. Compile + link wrapper in Docker.
echo "Linking Airwindows.clap..."
docker run --rm --platform linux/arm64 \
    -v "$REPO_ROOT:/repo" \
    -v "$BUILD_DIR:/work" \
    -w /work \
    "$IMAGE_NAME" bash -c "
        set -e
        g++ -O3 -shared -fPIC -std=c++17 \
            -fvisibility=hidden -fvisibility-inlines-hidden -DNDEBUG \
            /repo/scripts/airwindows_clap_wrapper.cpp \
            -I airwin2rack/src \
            -I /repo/third_party/clap/include \
            custom-build/cmake/libairwin-registry.a \
            custom-build/cmake/libawdoc_resources.a \
            -o custom-build/Airwindows.clap -ldl
        strip custom-build/Airwindows.clap
    "

# 6. Deploy to plugins/.
cp "$CLAP_OUT_DIR/Airwindows.clap" "$PLUGINS_DIR/Airwindows.clap"

echo ""
echo "=== Airwindows.clap built ==="
ls -la "$PLUGINS_DIR/Airwindows.clap"

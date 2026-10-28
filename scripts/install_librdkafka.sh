#!/bin/sh
set -eu

# Keep application images, development, and CI on the same tested release.
LIBRDKAFKA_COMMIT=c6c9973cf1b25394c58e0cade13a36f386856f77 # v2.16.0
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

git init --quiet "$build_dir/source"
git -C "$build_dir/source" fetch --depth 1 \
    https://github.com/confluentinc/librdkafka.git "$LIBRDKAFKA_COMMIT"
git -C "$build_dir/source" checkout --detach "$LIBRDKAFKA_COMMIT"
cmake -S "$build_dir/source" -B "$build_dir/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DRDKAFKA_BUILD_EXAMPLES=OFF \
    -DRDKAFKA_BUILD_TESTS=OFF \
    -DWITH_SSL=ON -DWITH_SASL=ON -DWITH_CURL=ON \
    -DWITH_ZLIB=ON -DWITH_ZSTD=ON -DENABLE_LZ4_EXT=ON
cmake --build "$build_dir/build" --parallel 2
cmake --install "$build_dir/build"

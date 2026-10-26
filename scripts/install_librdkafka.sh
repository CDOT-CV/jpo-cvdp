#!/bin/sh
set -eu

# Keep application images, development, and CI on the same tested release.
LIBRDKAFKA_VERSION=2.16.0
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

git clone --branch "v${LIBRDKAFKA_VERSION}" --depth 1 \
    https://github.com/confluentinc/librdkafka.git "$build_dir/source"
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

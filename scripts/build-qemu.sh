#!/bin/sh
set -eu

export PATH="/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
QEMU="$ROOT/build/qemu"
PREFIX="$ROOT/output/qemu"
mkdir -p "$PREFIX"

if [ ! -d "$QEMU" ]; then
  echo "Run ./scripts/fetch-sources.sh first." >&2
  exit 1
fi

if [ ! -f "$QEMU/build/build.ninja" ]; then
  cd "$QEMU"
  ./configure \
    --target-list=aarch64-softmmu \
    --prefix="$PREFIX" \
    --enable-slirp \
    --disable-werror
fi

ninja -C "$QEMU/build" -j"$(sysctl -n hw.ncpu)"
ninja -C "$QEMU/build" install
echo "QEMU installed at $PREFIX/bin/qemu-system-aarch64"

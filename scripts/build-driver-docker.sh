#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=${IMAGE:-qemu-virtio-lab-builder:ubuntu24.04}
BUILD_VOLUME=${BUILD_VOLUME:-qemu-virtio-lab-build}

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker is required." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "Docker Desktop is not running." >&2
  exit 1
fi

echo "Building virtio_custom.ko in Docker container..."
docker run --rm --platform linux/arm64 \
  --entrypoint /bin/sh \
  -v "$ROOT:/repo" \
  -v "$BUILD_VOLUME:/repo/build" \
  "$IMAGE" \
  -c 'cd /repo/guest-driver && make KDIR=/repo/build/linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-'

echo "Driver module built: $ROOT/guest-driver/virtio_custom.ko"

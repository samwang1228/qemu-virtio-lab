#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=${IMAGE:-qemu-virtio-lab-builder:ubuntu24.04}
BUILD_VOLUME=${BUILD_VOLUME:-qemu-virtio-lab-build}
OUTPUT_VOLUME=${OUTPUT_VOLUME:-qemu-virtio-lab-output}

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker is required." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "Docker Desktop is not running." >&2
  exit 1
fi

docker build --platform linux/arm64 -t "$IMAGE" "$ROOT"
docker volume create "$BUILD_VOLUME" >/dev/null
docker volume create "$OUTPUT_VOLUME" >/dev/null

if [ ! -f "$ROOT/.docker-sources-ready" ]; then
  echo "Fetching fresh sources in the Linux container..."
  docker run --rm --platform linux/arm64 \
    --entrypoint /bin/sh \
    -v "$ROOT:/repo" \
    -v "$BUILD_VOLUME:/repo/build" \
    "$IMAGE" \
    -c 'cd /repo && ./scripts/fetch-sources.sh'
  touch "$ROOT/.docker-sources-ready"
fi

docker run --rm --platform linux/arm64 \
  --entrypoint /bin/sh \
  -v "$ROOT:/repo" \
  -v "$BUILD_VOLUME:/repo/build" \
  -v "$OUTPUT_VOLUME:/repo/output" \
  "$IMAGE" \
  -c 'cd /repo && ./scripts/build-guest.sh'

mkdir -p "$ROOT/output/guest"
docker run --rm --platform linux/arm64 \
  --entrypoint /bin/sh \
  -v "$OUTPUT_VOLUME:/source:ro" \
  -v "$ROOT/output:/destination" \
  "$IMAGE" \
  -c 'cp /source/guest/Image /source/guest/rootfs.ext4 /destination/guest/'

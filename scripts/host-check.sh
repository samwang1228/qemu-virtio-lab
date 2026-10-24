#!/bin/sh
set -eu

if [ "$(uname -s)" != Darwin ] || [ "$(uname -m)" != arm64 ]; then
  echo "This lab targets macOS arm64." >&2
  exit 1
fi

missing=0
for tool in brew git make cmake meson ninja qemu-system-aarch64; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "missing: $tool"
    missing=1
  else
    echo "found: $tool -> $(command -v "$tool")"
  fi
done

if command -v docker >/dev/null 2>&1; then
  echo "found: docker -> $(command -v docker)"
else
  echo "missing: docker (required by scripts/build-guest-docker.sh)"
  missing=1
fi

if [ "$missing" -ne 0 ]; then
  echo "Install host prerequisites with: brew install qemu meson ninja pkg-config" >&2
  exit 1
fi

echo "host-check: OK"

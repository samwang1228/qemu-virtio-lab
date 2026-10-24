#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$ROOT/config/versions.env"
mkdir -p "$ROOT/build"

clone_at() {
  name=$1
  url=$2
  ref=$3
  destination=$ROOT/build/$name
  if [ -d "$destination/.git" ]; then
    echo "$name already exists"
    return
  fi
  git clone --depth 1 --branch "$ref" "$url" "$destination"
}

# macOS host only builds QEMU; Linux/Buildroot are fetched inside the Docker container
# (see scripts/build-guest-docker.sh) to avoid case-insensitive filename collisions.
case "$(uname -s)" in
  Darwin)
    clone_at qemu "$QEMU_URL" "$QEMU_VERSION"
    ;;
  *)
    clone_at linux "$LINUX_URL" "$LINUX_VERSION"
    clone_at buildroot "$BUILDROOT_URL" "$BUILDROOT_VERSION"
    ;;
esac
echo "sources: ready"

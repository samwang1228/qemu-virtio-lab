#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LINUX="$ROOT/build/linux"
BUILDROOT="$ROOT/build/buildroot"
OUT="$ROOT/output/guest"
if command -v nproc >/dev/null 2>&1; then
  JOBS=${JOBS:-$(nproc)}
else
  JOBS=${JOBS:-$(sysctl -n hw.ncpu)}
fi
MAKE=${MAKE:-make}

case "$(uname -s)" in
  Linux)
    KERNEL_MAKE_FLAGS="ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE:-aarch64-linux-gnu-}"
    ;;
  Darwin)
    echo "macOS native guest build is not supported; run ./scripts/build-guest-docker.sh instead." >&2
    exit 1
    ;;
  *)
    echo "Unsupported host: $(uname -s)" >&2
    exit 1
    ;;
esac
mkdir -p "$OUT"

if [ ! -d "$LINUX" ] || [ ! -d "$BUILDROOT" ]; then
  echo "Run ./scripts/fetch-sources.sh first." >&2
  exit 1
fi

"$MAKE" -C "$LINUX" $KERNEL_MAKE_FLAGS defconfig
"$MAKE" -C "$LINUX" $KERNEL_MAKE_FLAGS olddefconfig
scripts=$(mktemp)
trap 'rm -f "$scripts"' EXIT
cat "$ROOT/config/linux-aarch64.config" > "$scripts"
"$MAKE" -C "$LINUX" $KERNEL_MAKE_FLAGS KCONFIG_ALLCONFIG="$scripts" olddefconfig
"$LINUX/scripts/config" --file "$LINUX/.config" --disable MODULES
"$MAKE" -C "$LINUX" $KERNEL_MAKE_FLAGS olddefconfig
"$MAKE" -C "$LINUX" $KERNEL_MAKE_FLAGS -j"$JOBS" Image
cp "$LINUX/arch/arm64/boot/Image" "$OUT/Image"

"$MAKE" -C "$BUILDROOT" O="$OUT/buildroot" qemu_aarch64_virt_defconfig
cat "$ROOT/config/buildroot-aarch64.fragment" >> "$OUT/buildroot/.config"
"$MAKE" -C "$BUILDROOT" O="$OUT/buildroot" olddefconfig
"$MAKE" -C "$BUILDROOT" O="$OUT/buildroot" -j"$JOBS"
cp "$OUT/buildroot/images/rootfs.ext4" "$OUT/rootfs.ext4"
echo "guest artifacts are in $OUT"

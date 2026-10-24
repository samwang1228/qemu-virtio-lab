#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
QEMU="$ROOT/output/qemu/bin/qemu-system-aarch64"
[ -x "$QEMU" ] || QEMU=$(command -v qemu-system-aarch64 || true)
[ -n "$QEMU" ] || { echo "Build QEMU first." >&2; exit 1; }

[ -f "$ROOT/output/guest/Image" ] || { echo "Build guest first." >&2; exit 1; }
[ -f "$ROOT/output/guest/rootfs.ext4" ] || { echo "Build rootfs first." >&2; exit 1; }

set -- "$QEMU" \
  -machine virt,gic-version=3 \
  -cpu max \
  -smp 2 \
  -m 512M \
  -nographic \
  -kernel "$ROOT/output/guest/Image" \
  -append 'console=ttyAMA0 root=/dev/vda rw rootwait' \
  -drive "if=none,file=$ROOT/output/guest/rootfs.ext4,format=raw,id=hd0" \
  -device virtio-blk-pci,drive=hd0

if [ "${CUSTOM_DEVICE:-0}" = 1 ]; then
  set -- "$@" -device virtio-custom-device
fi

exec "$@"

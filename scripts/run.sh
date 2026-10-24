#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
QEMU="$ROOT/output/qemu/bin/qemu-system-aarch64"
[ -x "$QEMU" ] || QEMU=$(command -v qemu-system-aarch64 || true)
[ -n "$QEMU" ] || { echo "Build QEMU first." >&2; exit 1; }

[ -f "$ROOT/output/guest/Image" ] || { echo "Build guest first." >&2; exit 1; }
[ -f "$ROOT/output/guest/rootfs.ext4" ] || { echo "Build rootfs first." >&2; exit 1; }

SHARE_DIR=${SHARE_DIR:-$ROOT/guest-driver}

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

# 9p Virtio filesystem sharing (shares $SHARE_DIR as mount_tag "host_share")
if [ -d "$SHARE_DIR" ]; then
  set -- "$@" \
    -fsdev "local,id=fsdev0,path=$SHARE_DIR,security_model=none" \
    -device "virtio-9p-pci,fsdev=fsdev0,mount_tag=host_share"
fi

# Custom Virtio PCI device
if [ "${CUSTOM_DEVICE:-0}" = 1 ]; then
  set -- "$@" -device virtio-custom-pci
fi

echo "Starting QEMU... (quit with Ctrl-a x)"
if [ -d "$SHARE_DIR" ]; then
  echo "9p share available. Mount inside guest with:"
  echo "  mkdir -p /mnt && mount -t 9p -o trans=virtio host_share /mnt"
fi

exec "$@"

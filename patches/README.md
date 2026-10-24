# QEMU Custom Virtio Device

The custom Virtio device is implemented for QEMU v9.2.0:

- **Virtio ID**: `VIRTIO_ID_CUSTOM 43` (PCI device ID `0x106b` in modern Virtio-1.0 namespace).
- **QEMU Files**:
  - `include/hw/virtio/virtio-custom.h`: Type definitions and config struct.
  - `hw/virtio/virtio-custom.c`: Device model with a single virtqueue and `write_count` counter in config space.
  - `hw/virtio/virtio-custom-pci.c`: Modern PCIe proxy (`virtio-custom-pci`).
- **Patch file**: `patches/0001-virtio-custom-device.patch` contains the complete diff against QEMU v9.2.0.

### Applying the patch to a fresh QEMU tree

```sh
cd build/qemu
git apply ../../patches/0001-virtio-custom-device.patch
ninja -C build install
```

### Running with the custom device

```sh
CUSTOM_DEVICE=1 ./scripts/run.sh
```

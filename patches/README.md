# QEMU Custom Virtio Device

The custom Virtio DMA engine device is implemented for QEMU `v9.2.0`:

- **Virtio / PCI Identity**: `VIRTIO_ID_CUSTOM 43` (modern Virtio-1.0 PCI Device ID `0x1040 + 43 = 0x106b`, Vendor ID `0x1af4` / `PCI_VENDOR_ID_REDHAT_QUMRANET`).
- **Shared UAPI / ABI**: [`include/uapi_virtio_custom.h`](../include/uapi_virtio_custom.h) (symlinked at `include/standard-headers/linux/virtio_custom.h` in QEMU) acts as the single source of truth for both QEMU hardware and the Linux guest driver (`guest-driver/`), defining:
  - **DMA opcodes**: `VIRTIO_CUSTOM_DMA_COPY` (`0`), `VIRTIO_CUSTOM_DMA_FILL` (`1`), `VIRTIO_CUSTOM_DMA_INVERT` (`2`)
  - **Status codes**: `VIRTIO_CUSTOM_STATUS_OK` (`0`), `VIRTIO_CUSTOM_STATUS_EINVAL` (`1`), `VIRTIO_CUSTOM_STATUS_ENOMEM` (`2`), `VIRTIO_CUSTOM_STATUS_EIO` (`3`)
  - **Descriptors & config space**: `struct virtio_custom_dma_cmd`, `struct virtio_custom_dma_resp`, and `struct virtio_custom_config` (`write_count`, `dma_transfers`, `dma_bytes`)
- **QEMU Files in `patches/0001-virtio-custom-device.patch`**:
  - `include/hw/virtio/virtio-custom.h`: QOM type declarations (`virtio-custom-device` / `TYPE_VIRTIO_CUSTOM`, `virtio-custom-pci-base` / `TYPE_VIRTIO_CUSTOM_PCI`), `VirtIOCustom` / `VirtIOCustomPCI` structs, and `VirtIOCustomConfig` typedef.
  - `hw/virtio/virtio-custom.c`: DMA engine device model with a 128-entry virtqueue (`virtio_custom_handle_output()`), `dma_memory_read()` / `dma_memory_write()` over `vdev->dma_as` supporting `COPY`/`FILL`/`INVERT`, 16 MiB length bounds check (`VIRTIO_CUSTOM_MAX_DMA_LEN`), `MemTxResult` error mapping, legacy `write_count` fallback path, config space counters (`write_count`, `dma_transfers`, `dma_bytes`), and `VMStateDescription` (`vmstate_virtio_custom`, `version_id = 2`).
  - `hw/virtio/virtio-custom-pci.c`: Modern Virtio-1.0 PCIe proxy (`virtio-custom-pci` and `virtio-custom-pci-non-transitional`) defaulting to 2 MSI-X vectors (`nvectors = 2`) with the `ioeventfd` property (`VIRTIO_PCI_FLAG_USE_IOEVENTFD_BIT`).
  - `hw/virtio/meson.build`, `hw/virtio/virtio.c`, `include/standard-headers/linux/virtio_ids.h`: Build integration, `virtio_device_names[]` entry (`"virtio-custom"`), and `VIRTIO_ID_CUSTOM` registration.
- **Patch file**: [`patches/0001-virtio-custom-device.patch`](0001-virtio-custom-device.patch) contains the complete diff against QEMU `v9.2.0`.

## Applying the patch to a fresh QEMU tree

```sh
cd build/qemu
git apply ../../patches/0001-virtio-custom-device.patch
ninja -C build install
```

## Running and verifying with the custom device

1. Build the out-of-tree Linux kernel driver module (`virtio_custom.ko`) in Docker:

```sh
./scripts/build-driver-docker.sh
```

2. Boot QEMU with the custom device and Virtio-9P host directory sharing:

```sh
CUSTOM_DEVICE=1 ./scripts/run.sh
```

3. Inside the guest (log in as `root`), mount the 9P share, load the module, and inspect self-test and hardware config counters via sysfs:

```sh
mkdir -p /mnt
mount -t 9p -o trans=virtio host_share /mnt
insmod /mnt/virtio_custom.ko
dmesg | tail -n 25
cat /sys/bus/virtio/devices/virtio*/dma_test
cat /sys/bus/virtio/devices/virtio*/dma_stats
```

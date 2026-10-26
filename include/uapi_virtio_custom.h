/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Virtio Custom Device - Shared Protocol Specification & UAPI
 *
 * This header defines the binary interface (ABI) between the QEMU virtual
 * hardware device and the Linux guest kernel / user space driver.
 */

#ifndef _UAPI_VIRTIO_CUSTOM_H
#define _UAPI_VIRTIO_CUSTOM_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/*
 * Virtio Device ID for Custom Device
 * 43 is unallocated in modern Virtio specs.
 * Under modern PCI transport, PCI Device ID = 0x1040 + 43 = 0x106b.
 */
#ifndef VIRTIO_ID_CUSTOM
#define VIRTIO_ID_CUSTOM 43
#endif

/*
 * DMA Engine Operation Codes (Opcodes)
 */
#define VIRTIO_CUSTOM_DMA_COPY   0  /* Read GPA src -> Write GPA dst */
#define VIRTIO_CUSTOM_DMA_FILL   1  /* Fill GPA dst with 8-bit pattern */
#define VIRTIO_CUSTOM_DMA_INVERT 2  /* Read GPA src -> Bitwise NOT -> Write GPA dst */

/*
 * DMA Status Codes
 */
#define VIRTIO_CUSTOM_STATUS_OK       0
#define VIRTIO_CUSTOM_STATUS_EINVAL   1
#define VIRTIO_CUSTOM_STATUS_ENOMEM   2
#define VIRTIO_CUSTOM_STATUS_EIO      3

/*
 * DMA Command Descriptor (Out-buffer: Host reads from Guest)
 *
 * Passed via Virtqueue to instruct the hardware DMA engine to perform
 * bulk physical memory read/write.
 */
struct virtio_custom_dma_cmd {
	uint32_t opcode;
	uint32_t len;
	uint64_t src_addr;    /* Guest Physical Address (GPA) */
	uint64_t dst_addr;    /* Guest Physical Address (GPA) */
	uint32_t pattern;
} __attribute__((packed));

/*
 * DMA Response Descriptor (In-buffer: Host writes to Guest)
 */
struct virtio_custom_dma_resp {
	uint32_t status;       /* VIRTIO_CUSTOM_STATUS_* */
	uint32_t transferred;  /* Actual bytes transferred */
} __attribute__((packed));

/*
 * Device Configuration Space (MMIO / PCI BAR configuration)
 */
struct virtio_custom_config {
	uint64_t write_count;
	uint64_t dma_transfers;
	uint64_t dma_bytes;
} __attribute__((packed));

#endif /* _UAPI_VIRTIO_CUSTOM_H */

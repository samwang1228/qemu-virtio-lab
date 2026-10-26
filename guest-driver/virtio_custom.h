/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Virtio Custom Driver - Internal Header
 */

#ifndef _VIRTIO_CUSTOM_H
#define _VIRTIO_CUSTOM_H

#include <linux/types.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/device.h>

#include <uapi_virtio_custom.h>

/*
 * Driver private state for Virtio Custom device
 */
struct virtio_custom {
	struct virtio_device *vdev;
	struct virtqueue *vq;
	struct completion dma_done;
	spinlock_t lock;
};

/*
 * DMA Subsystem API (virtio_custom_dma.c)
 */
int virtio_custom_dma_init(struct virtio_custom *custom);
void virtio_custom_dma_cleanup(struct virtio_custom *custom);
int virtio_custom_do_dma(struct virtio_custom *custom,
			 uint32_t opcode,
			 uint64_t src_phys,
			 uint64_t dst_phys,
			 uint32_t len,
			 uint32_t pattern,
			 struct virtio_custom_dma_resp *out_resp);
int virtio_custom_run_self_test(struct virtio_custom *custom);

/*
 * Sysfs Interface Subsystem (virtio_custom_sysfs.c)
 */
int virtio_custom_sysfs_init(struct virtio_custom *custom);
void virtio_custom_sysfs_cleanup(struct virtio_custom *custom);

#endif /* _VIRTIO_CUSTOM_H */

// SPDX-License-Identifier: GPL-2.0
/*
 * Virtio Custom Driver - Sysfs Interface Subsystem
 */

#include <linux/device.h>
#include <linux/stddef.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>

#include "virtio_custom.h"

static ssize_t dma_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virtio_device *vdev = dev_to_virtio(dev);
	struct virtio_custom *custom = vdev->priv;
	int ret = virtio_custom_run_self_test(custom);

	return sysfs_emit(buf, "DMA self-test status: %d (0 = success)\n", ret);
}
static DEVICE_ATTR_RO(dma_test);

static ssize_t dma_stats_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virtio_device *vdev = dev_to_virtio(dev);
	u64 write_count = virtio_cread64(vdev, offsetof(struct virtio_custom_config, write_count));
	u64 dma_transfers = virtio_cread64(vdev, offsetof(struct virtio_custom_config, dma_transfers));
	u64 dma_bytes = virtio_cread64(vdev, offsetof(struct virtio_custom_config, dma_bytes));

	return sysfs_emit(buf,
			  "write_count: %llu\ndma_transfers: %llu\ndma_bytes: %llu\n",
			  (unsigned long long)write_count,
			  (unsigned long long)dma_transfers,
			  (unsigned long long)dma_bytes);
}
static DEVICE_ATTR_RO(dma_stats);

int virtio_custom_sysfs_init(struct virtio_custom *custom)
{
	int ret;

	ret = device_create_file(&custom->vdev->dev, &dev_attr_dma_test);
	if (ret) {
		dev_warn(&custom->vdev->dev, "Failed to create sysfs dma_test attribute\n");
		return ret;
	}

	ret = device_create_file(&custom->vdev->dev, &dev_attr_dma_stats);
	if (ret) {
		dev_warn(&custom->vdev->dev, "Failed to create sysfs dma_stats attribute\n");
		device_remove_file(&custom->vdev->dev, &dev_attr_dma_test);
		return ret;
	}

	return 0;
}

void virtio_custom_sysfs_cleanup(struct virtio_custom *custom)
{
	device_remove_file(&custom->vdev->dev, &dev_attr_dma_stats);
	device_remove_file(&custom->vdev->dev, &dev_attr_dma_test);
}

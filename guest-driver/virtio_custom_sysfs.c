// SPDX-License-Identifier: GPL-2.0
/*
 * Virtio Custom Driver - Sysfs Interface Subsystem
 */

#include <linux/device.h>
#include <linux/virtio.h>

#include "virtio_custom.h"

static ssize_t dma_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virtio_device *vdev = dev_to_virtio(dev);
	struct virtio_custom *custom = vdev->priv;
	int ret = virtio_custom_run_self_test(custom);

	return sysfs_emit(buf, "DMA self-test status: %d (0 = success)\n", ret);
}
static DEVICE_ATTR_RO(dma_test);

int virtio_custom_sysfs_init(struct virtio_custom *custom)
{
	int ret;

	ret = device_create_file(&custom->vdev->dev, &dev_attr_dma_test);
	if (ret)
		dev_warn(&custom->vdev->dev, "Failed to create sysfs dma_test attribute\n");

	return ret;
}

void virtio_custom_sysfs_cleanup(struct virtio_custom *custom)
{
	device_remove_file(&custom->vdev->dev, &dev_attr_dma_test);
}

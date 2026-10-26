// SPDX-License-Identifier: GPL-2.0
/*
 * Virtio Custom Driver - Main Entry Point & Bus Lifecycle
 */

#include <linux/module.h>
#include <linux/virtio.h>

#include "virtio_custom.h"

static int virtio_custom_probe(struct virtio_device *vdev)
{
	struct virtio_custom *custom;
	int ret;

	custom = devm_kzalloc(&vdev->dev, sizeof(*custom), GFP_KERNEL);
	if (!custom)
		return -ENOMEM;

	custom->vdev = vdev;
	vdev->priv = custom;

	/* Initialize DMA & Virtqueue subsystem */
	ret = virtio_custom_dma_init(custom);
	if (ret) {
		dev_err(&vdev->dev, "Failed to initialize DMA virtqueue: %d\n", ret);
		return ret;
	}

	virtio_device_ready(vdev);
	dev_info(&vdev->dev, "custom virtio DMA device attached (id=%d)\n", vdev->id.device);

	/* Initialize Sysfs interface */
	virtio_custom_sysfs_init(custom);

	/* Automatically run self-test on attach */
	virtio_custom_run_self_test(custom);

	return 0;
}

static void virtio_custom_remove(struct virtio_device *vdev)
{
	struct virtio_custom *custom = vdev->priv;

	virtio_custom_sysfs_cleanup(custom);
	virtio_custom_dma_cleanup(custom);

	dev_info(&vdev->dev, "custom virtio device removed\n");
}

static const struct virtio_device_id virtio_custom_id_table[] = {
	{ VIRTIO_ID_CUSTOM, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_custom_driver = {
	.driver.name = "virtio_custom",
	.id_table = virtio_custom_id_table,
	.probe = virtio_custom_probe,
	.remove = virtio_custom_remove,
};

module_virtio_driver(virtio_custom_driver);

MODULE_DEVICE_TABLE(virtio, virtio_custom_id_table);
MODULE_DESCRIPTION("Virtio custom DMA engine driver");
MODULE_LICENSE("GPL");

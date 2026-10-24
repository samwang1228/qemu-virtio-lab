// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>

#define VIRTIO_ID_CUSTOM 0x105a

struct virtio_custom {
	struct virtio_device *vdev;
	struct virtqueue *vq;
};

static int virtio_custom_probe(struct virtio_device *vdev)
{
	struct virtio_custom *custom;
	int ret;

	custom = devm_kzalloc(&vdev->dev, sizeof(*custom), GFP_KERNEL);
	if (!custom)
		return -ENOMEM;

	custom->vdev = vdev;
	ret = virtio_find_single_vq(vdev, &custom->vq, NULL, "custom");
	if (ret)
		return ret;

	virtio_device_ready(vdev);
	dev_info(&vdev->dev, "custom virtio device attached\n");
	return 0;
}

static void virtio_custom_remove(struct virtio_device *vdev)
{
	virtio_reset_device(vdev);
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
MODULE_DESCRIPTION("Virtio custom device learning driver");
MODULE_LICENSE("GPL");

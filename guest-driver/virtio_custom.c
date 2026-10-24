// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>

#define VIRTIO_ID_CUSTOM 43

struct virtio_custom {
	struct virtio_device *vdev;
	struct virtqueue *vq;
};

static void virtio_custom_vq_callback(struct virtqueue *vq)
{
	struct virtio_custom *custom = vq->vdev->priv;
	unsigned int len;
	void *buf;

	while ((buf = virtqueue_get_buf(custom->vq, &len)) != NULL) {
		dev_info(&custom->vdev->dev, "completed buffer, len=%u\n", len);
	}
}

static int virtio_custom_probe(struct virtio_device *vdev)
{
	struct virtio_custom *custom;

	custom = devm_kzalloc(&vdev->dev, sizeof(*custom), GFP_KERNEL);
	if (!custom)
		return -ENOMEM;

	custom->vdev = vdev;
	vdev->priv = custom;

	custom->vq = virtio_find_single_vq(vdev, virtio_custom_vq_callback, "custom");
	if (IS_ERR(custom->vq))
		return PTR_ERR(custom->vq);

	virtio_device_ready(vdev);
	dev_info(&vdev->dev, "custom virtio device attached (id=%d)\n", vdev->id.device);
	return 0;
}

static void virtio_custom_remove(struct virtio_device *vdev)
{
	virtio_reset_device(vdev);
	vdev->config->del_vqs(vdev);
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

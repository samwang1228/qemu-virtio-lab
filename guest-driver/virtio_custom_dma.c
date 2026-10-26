// SPDX-License-Identifier: GPL-2.0
/*
 * Virtio Custom Driver - DMA & Virtqueue Engine Subsystem
 */

#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/jiffies.h>

#include "virtio_custom.h"

static void virtio_custom_vq_callback(struct virtqueue *vq)
{
	struct virtio_custom *custom = vq->vdev->priv;
	unsigned int len;
	void *buf;
	unsigned long flags;

	spin_lock_irqsave(&custom->lock, flags);
	while ((buf = virtqueue_get_buf(custom->vq, &len)) != NULL) {
		complete(&custom->dma_done);
	}
	spin_unlock_irqrestore(&custom->lock, flags);
}

int virtio_custom_dma_init(struct virtio_custom *custom)
{
	init_completion(&custom->dma_done);
	spin_lock_init(&custom->lock);

	custom->vq = virtio_find_single_vq(custom->vdev, virtio_custom_vq_callback, "custom");
	if (IS_ERR(custom->vq))
		return PTR_ERR(custom->vq);

	return 0;
}

void virtio_custom_dma_cleanup(struct virtio_custom *custom)
{
	virtio_reset_device(custom->vdev);
	custom->vdev->config->del_vqs(custom->vdev);
}

int virtio_custom_do_dma(struct virtio_custom *custom,
			 uint32_t opcode,
			 uint64_t src_phys,
			 uint64_t dst_phys,
			 uint32_t len,
			 uint32_t pattern,
			 struct virtio_custom_dma_resp *out_resp)
{
	struct virtio_custom_dma_cmd *cmd;
	struct virtio_custom_dma_resp *resp;
	struct scatterlist out_sg, in_sg;
	struct scatterlist *sgs[2];
	unsigned long flags;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!cmd || !resp) {
		kfree(cmd);
		kfree(resp);
		return -ENOMEM;
	}

	cmd->opcode = opcode;
	cmd->len = len;
	cmd->src_addr = src_phys;
	cmd->dst_addr = dst_phys;
	cmd->pattern = pattern;

	sg_init_one(&out_sg, cmd, sizeof(*cmd));
	sg_init_one(&in_sg, resp, sizeof(*resp));
	sgs[0] = &out_sg;
	sgs[1] = &in_sg;

	reinit_completion(&custom->dma_done);

	spin_lock_irqsave(&custom->lock, flags);
	ret = virtqueue_add_sgs(custom->vq, sgs, 1, 1, cmd, GFP_ATOMIC);
	if (ret) {
		spin_unlock_irqrestore(&custom->lock, flags);
		kfree(cmd);
		kfree(resp);
		return ret;
	}
	virtqueue_kick(custom->vq);
	spin_unlock_irqrestore(&custom->lock, flags);

	if (!wait_for_completion_timeout(&custom->dma_done, msecs_to_jiffies(1000))) {
		dev_err(&custom->vdev->dev, "DMA transfer timed out!\n");
		kfree(cmd);
		kfree(resp);
		return -ETIMEDOUT;
	}

	if (out_resp)
		*out_resp = *resp;

	kfree(cmd);
	kfree(resp);
	return 0;
}

int virtio_custom_run_self_test(struct virtio_custom *custom)
{
	struct device *dev = &custom->vdev->dev;
	char *src, *dst;
	phys_addr_t src_phys, dst_phys;
	struct virtio_custom_dma_resp resp;
	const char *test_msg = "Hello Virtio DMA: Direct Physical Memory Transfer Works!";
	uint32_t len = strlen(test_msg) + 1;
	int ret;

	src = kzalloc(len, GFP_KERNEL);
	dst = kzalloc(len, GFP_KERNEL);
	if (!src || !dst) {
		kfree(src);
		kfree(dst);
		return -ENOMEM;
	}

	memcpy(src, test_msg, len);
	src_phys = virt_to_phys(src);
	dst_phys = virt_to_phys(dst);

	dev_info(dev, "=== Running Virtio DMA Self-Test ===\n");
	dev_info(dev, "Allocated Guest RAM: src GPA=0x%llx, dst GPA=0x%llx, size=%u bytes\n",
		 (unsigned long long)src_phys, (unsigned long long)dst_phys, len);

	/* Test 1: DMA Copy */
	dev_info(dev, "[Test 1/2] Executing DMA COPY (Device reading GPA src -> writing GPA dst)...\n");
	ret = virtio_custom_do_dma(custom, VIRTIO_CUSTOM_DMA_COPY,
				   src_phys, dst_phys, len, 0, &resp);
	if (ret) {
		dev_err(dev, "DMA COPY failed: %d\n", ret);
		goto out;
	}

	if (memcmp(src, dst, len) == 0) {
		dev_info(dev, "[Test 1/2] SUCCESS! Destination buffer contains: \"%s\"\n", dst);
	} else {
		dev_err(dev, "[Test 1/2] FAILED: Memory mismatch!\n");
		ret = -EINVAL;
		goto out;
	}

	/* Test 2: DMA Invert */
	dev_info(dev, "[Test 2/2] Executing DMA BITWISE INVERT...\n");
	memset(dst, 0, len);
	ret = virtio_custom_do_dma(custom, VIRTIO_CUSTOM_DMA_INVERT,
				   src_phys, dst_phys, len, 0, &resp);
	if (ret) {
		dev_err(dev, "DMA INVERT failed: %d\n", ret);
		goto out;
	}

	if ((unsigned char)dst[0] == (unsigned char)(~src[0])) {
		dev_info(dev, "[Test 2/2] SUCCESS! First byte inverted: 0x%02x -> 0x%02x\n",
			 (unsigned char)src[0], (unsigned char)dst[0]);
	} else {
		dev_err(dev, "[Test 2/2] FAILED: Invert mismatch!\n");
		ret = -EINVAL;
		goto out;
	}

	dev_info(dev, "=== Virtio DMA Engine Verified Successfully! ===\n");

out:
	kfree(src);
	kfree(dst);
	return ret;
}

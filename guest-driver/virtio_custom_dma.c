// SPDX-License-Identifier: GPL-2.0
/*
 * Virtio Custom Driver - DMA & Virtqueue Engine Subsystem
 */

#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>

#include "virtio_custom.h"

static struct kmem_cache *virtio_custom_req_cache;

int virtio_custom_dma_slab_init(void)
{
	virtio_custom_req_cache = kmem_cache_create("virtio_custom_req",
						    sizeof(struct virtio_custom_req),
						    0,
						    SLAB_HWCACHE_ALIGN,
						    NULL);
	if (!virtio_custom_req_cache)
		return -ENOMEM;

	return 0;
}

void virtio_custom_dma_slab_exit(void)
{
	kmem_cache_destroy(virtio_custom_req_cache);
}

static void virtio_custom_req_release(struct kref *ref)
{
	struct virtio_custom_req *req = container_of(ref, struct virtio_custom_req, ref);

	kmem_cache_free(virtio_custom_req_cache, req);
}

static void virtio_custom_vq_callback(struct virtqueue *vq)
{
	struct virtio_custom *custom = vq->vdev->priv;
	struct virtio_custom_req *req;
	unsigned int len;
	unsigned long flags;

	spin_lock_irqsave(&custom->lock, flags);
	while ((req = virtqueue_get_buf(custom->vq, &len)) != NULL) {
		complete(&req->done);
		kref_put(&req->ref, virtio_custom_req_release);
	}
	spin_unlock_irqrestore(&custom->lock, flags);
}

int virtio_custom_dma_init(struct virtio_custom *custom)
{
	spin_lock_init(&custom->lock);

	custom->vq = virtio_find_single_vq(custom->vdev, virtio_custom_vq_callback, "custom");
	if (IS_ERR(custom->vq))
		return PTR_ERR(custom->vq);

	return 0;
}

void virtio_custom_dma_cleanup(struct virtio_custom *custom)
{
	struct virtio_custom_req *req;

	/* Quiesce hardware before reclaiming in-flight buffers */
	virtio_reset_device(custom->vdev);

	/* Drop device references for any timed-out requests still on the ring */
	while ((req = virtqueue_detach_unused_buf(custom->vq)) != NULL) {
		kref_put(&req->ref, virtio_custom_req_release);
	}

	custom->vdev->config->del_vqs(custom->vdev);
}

int virtio_custom_do_dma(struct virtio_custom *custom,
			 uint32_t opcode,
			 dma_addr_t src_dma,
			 dma_addr_t dst_dma,
			 uint32_t len,
			 uint32_t pattern,
			 struct virtio_custom_dma_resp *out_resp)
{
	struct virtio_custom_req *req;
	struct scatterlist out_sg, in_sg;
	struct scatterlist *sgs[2];
	unsigned long flags;
	bool notify;
	int ret;

	req = kmem_cache_zalloc(virtio_custom_req_cache, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	init_completion(&req->done);
	kref_init(&req->ref);

	req->cmd.opcode = opcode;
	req->cmd.len = len;
	req->cmd.src_addr = (uint64_t)src_dma;
	req->cmd.dst_addr = (uint64_t)dst_dma;
	req->cmd.pattern = pattern;

	sg_init_one(&out_sg, &req->cmd, sizeof(req->cmd));
	sg_init_one(&in_sg, &req->resp, sizeof(req->resp));
	sgs[0] = &out_sg;
	sgs[1] = &in_sg;

	spin_lock_irqsave(&custom->lock, flags);
	/* Take second reference for the Virtqueue / device ownership */
	kref_get(&req->ref);
	ret = virtqueue_add_sgs(custom->vq, sgs, 1, 1, req, GFP_ATOMIC);
	if (ret) {
		kref_put(&req->ref, virtio_custom_req_release);
		spin_unlock_irqrestore(&custom->lock, flags);
		kref_put(&req->ref, virtio_custom_req_release);
		return ret;
	}
	notify = virtqueue_kick_prepare(custom->vq);
	spin_unlock_irqrestore(&custom->lock, flags);

	if (notify)
		virtqueue_notify(custom->vq);

	if (!wait_for_completion_timeout(&req->done, msecs_to_jiffies(1000))) {
		dev_err(&custom->vdev->dev, "DMA transfer timed out!\n");
		/*
		 * Drop caller's reference only. The request memory remains valid
		 * until either the device completes it later or virtio_custom_dma_cleanup()
		 * detaches unused buffers after a device reset, preventing Use-After-Free.
		 */
		kref_put(&req->ref, virtio_custom_req_release);
		return -ETIMEDOUT;
	}

	if (out_resp)
		*out_resp = req->resp;

	switch (req->resp.status) {
	case VIRTIO_CUSTOM_STATUS_OK:
		ret = 0;
		break;
	case VIRTIO_CUSTOM_STATUS_EINVAL:
		ret = -EINVAL;
		break;
	case VIRTIO_CUSTOM_STATUS_ENOMEM:
		ret = -ENOMEM;
		break;
	case VIRTIO_CUSTOM_STATUS_EIO:
	default:
		ret = -EIO;
		break;
	}

	kref_put(&req->ref, virtio_custom_req_release);
	return ret;
}

int virtio_custom_run_self_test(struct virtio_custom *custom)
{
	struct device *dev = &custom->vdev->dev;
	/*
	 * In the Linux Virtio bus model, the underlying PCI/MMIO transport parent
	 * device owns the DMA mask and IOMMU configuration.
	 */
	struct device *dma_dev = custom->vdev->dev.parent;
	char *src = NULL, *dst = NULL;
	dma_addr_t src_dma = 0, dst_dma = 0;
	struct virtio_custom_dma_resp resp;
	const char *test_msg = "Hello Virtio DMA: Direct Physical Memory Transfer Works!";
	uint32_t len = strlen(test_msg) + 1;
	int ret = 0;

	src = dma_alloc_coherent(dma_dev, len, &src_dma, GFP_KERNEL);
	dst = dma_alloc_coherent(dma_dev, len, &dst_dma, GFP_KERNEL);
	if (!src || !dst) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(src, test_msg, len);

	dev_info(dev, "=== Running Virtio DMA Self-Test ===\n");
	dev_info(dev, "Allocated Coherent DMA RAM: src=0x%llx, dst=0x%llx, size=%u bytes\n",
		 (unsigned long long)src_dma, (unsigned long long)dst_dma, len);

	/* Test 1: DMA Copy */
	dev_info(dev, "[Test 1/2] Executing DMA COPY (Device reading src -> writing dst)...\n");
	ret = virtio_custom_do_dma(custom, VIRTIO_CUSTOM_DMA_COPY,
				   src_dma, dst_dma, len, 0, &resp);
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
				   src_dma, dst_dma, len, 0, &resp);
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
	if (src)
		dma_free_coherent(dma_dev, len, src, src_dma);
	if (dst)
		dma_free_coherent(dma_dev, len, dst, dst_dma);
	return ret;
}

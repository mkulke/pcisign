/* pcisign_ioctl.c */
#include "pcisign_drv.h"
#include "pcisign_ioctl.h"

#define REG_CTRL    0x00
#define REG_STATUS  0x04
#define REG_SRC_LO  0x08
#define REG_SRC_HI  0x0c
#define REG_SRC_LEN 0x10
#define REG_DST_LO  0x14
#define REG_DST_HI  0x18
#define REG_DST_LEN 0x1c
#define REG_ALGO    0x20

#define CTRL_START     0x1
#define CTRL_CMD_SHIFT 8
#define CTRL_CMD_MASK  (0xff << CTRL_CMD_SHIFT)
#define CMD_TEST       0x1
#define CMD_SIGN       0x2

#define STATUS_DONE 0x1
#define STATUS_ERR  0x2

static long test_cmd(struct pcisign_dev *d)
{
	u32 ctrl = CTRL_START | (CMD_TEST << CTRL_CMD_SHIFT);

	reinit_completion(&d->done);

	/* write CTRL_START w/ CMD_TEST at offset 0 for test */
	writel(ctrl, d->mmio + REG_CTRL);

	if (wait_for_completion_timeout(&d->done, msecs_to_jiffies(1000)) == 0) {
		pr_err("pcisign: TEST timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static long sign_cmd(struct pcisign_dev *d, struct pcisign_req *req)
{
	void *src_ptr = (void*) req->src_ptr;
	void *dst_ptr = (void*) req->dst_ptr;
	u32 src_len = req->src_len;
	u32 dst_len = req->dst_len;
	u32 algo = req->algo;
	struct device *dev = &d->pdev->dev;
	u32 ctrl, status;
	dma_addr_t dma_src, dma_dst;
	void *src_kbuf = NULL, *dst_kbuf = NULL;
	int ret = 0;

	src_kbuf = kmalloc(src_len, GFP_KERNEL);
	dst_kbuf = kmalloc(dst_len, GFP_KERNEL);
	if (!src_kbuf || !dst_kbuf) {
		ret = -ENOMEM;
		goto out;
	}

    /* copy data from userspace to kernel buffer */
    if (copy_from_user(src_kbuf, src_ptr, src_len)) {
        ret = -EFAULT;
        goto out;
    }

	/* map buffers for DMA */
	dma_src = dma_map_single(dev, src_kbuf, src_len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dma_src)) {
        ret = -ENOMEM;
        goto out;
    }
	dma_dst = dma_map_single(dev, dst_kbuf, dst_len, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dma_dst)) {
		dma_unmap_single(dev, dma_src, src_len, DMA_TO_DEVICE);
		ret = -ENOMEM;
		goto out;
	}

    /* program device MMIO registers with params */
    writel(lower_32_bits(dma_src), d->mmio + REG_SRC_LO);
    writel(upper_32_bits(dma_src), d->mmio + REG_SRC_HI);
    writel(src_len,                d->mmio + REG_SRC_LEN);

    writel(lower_32_bits(dma_dst), d->mmio + REG_DST_LO);
    writel(upper_32_bits(dma_dst), d->mmio + REG_DST_HI);
    writel(dst_len,                d->mmio + REG_DST_LEN);

    writel(algo,                   d->mmio + REG_ALGO);

    reinit_completion(&d->done);

	/* write START and CMD bits to CTRL register */
	ctrl = CTRL_START | (CMD_SIGN << CTRL_CMD_SHIFT);
    writel(ctrl, d->mmio + REG_CTRL);

    /* sleep until ISR wakes us */
    wait_for_completion_interruptible(&d->done);

    dma_unmap_single(dev, dma_src, src_len, DMA_TO_DEVICE);
    dma_unmap_single(dev, dma_dst, dst_len, DMA_FROM_DEVICE);

	status = readl(d->mmio + REG_STATUS);
	if (status & STATUS_ERR) {
		pr_err("pcisign: SIGN failed, STATUS=0x%02x\n", status);
		ret = -EIO;
		goto out;
	}

	/* copy result back to userspace */
	if (copy_to_user(dst_ptr, dst_kbuf, dst_len))
		ret = -EFAULT;
out:
    kfree(src_kbuf);
    kfree(dst_kbuf);
	return ret;
}

long pcisign_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct miscdevice *m = f->private_data;
    struct pcisign_dev *d = container_of(m, struct pcisign_dev, miscdev);
	struct pcisign_req req;
	long ret;

    switch (cmd) {
    case PCISIGN_IOC_TEST:
		ret = test_cmd(d);
		if (ret)
			return ret;
		break;
	case PCISIGN_IOC_SIGN:
		if (copy_from_user(&req, (void*)arg, sizeof(req)))
			return -EFAULT;
		ret = sign_cmd(d, &req);
		if (ret)
			return ret;
		break;
    default:
        return -ENOTTY;
    }

    return 0;
}

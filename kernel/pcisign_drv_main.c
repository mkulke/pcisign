/* pcisign_drv_main.c */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/io.h>

#include "pcisign_drv.h"

#define PCISIGN_VENDOR 0x1234
#define PCISIGN_DEVICE 0xCAFE

#define REG_CTRL   0x00
#define REG_STATUS 0x04

static irqreturn_t pcisign_isr(int irq, void *dev_id)
{
    struct pcisign_dev *d = dev_id;
    u32 status = readl(d->mmio + REG_STATUS);

    pr_info("pcisign: IRQ, STATUS=0x%02x\n", status);

    complete(&d->done);
    return IRQ_HANDLED;
}

static int pcisign_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct pcisign_dev *d;
    int ret;

    d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;

    d->pdev = pdev;
    pci_set_drvdata(pdev, d);

    ret = pci_enable_device_mem(pdev);
    if (ret)
        return ret;

    /* Needed for DMA */
    pci_set_master(pdev);

    ret = pci_request_mem_regions(pdev, "pcisign");
    if (ret)
        goto err_disable;

    d->mmio = pci_iomap(pdev, 0, 0);    /* BAR0 */
    if (!d->mmio) {
        ret = -ENODEV;
        goto err_regions;
    }

    /* MSI-X only: request one vector */
    ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSIX);
    if (ret < 0) {
        pr_err("pcisign: pci_alloc_irq_vectors failed: %d\n", ret);
        goto err_iounmap;
    }

    d->irq = pci_irq_vector(pdev, 0);
    ret = request_irq(d->irq, pcisign_isr, 0, "pcisign", d);
    if (ret) {
        pr_err("pcisign: request_irq failed: %d\n", ret);
        goto err_vectors;
    }

    init_completion(&d->done);
    d->miscdev.minor = MISC_DYNAMIC_MINOR;
    d->miscdev.name  = "pcisign";
    d->miscdev.fops  = &pcisign_fops;
    ret = misc_register(&d->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "misc_register failed: %d\n", ret);
        goto err_vectors;
    }

    pr_info("pcisign: probed, mmio=%p, irq=%d\n", d->mmio, d->irq);
    return 0;

err_vectors:
    pci_free_irq_vectors(pdev);
err_iounmap:
    pci_iounmap(pdev, d->mmio);
err_regions:
    pci_release_mem_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return ret;
}

static void pcisign_remove(struct pci_dev *pdev)
{
    struct pcisign_dev *d = pci_get_drvdata(pdev);

    misc_deregister(&d->miscdev);
    free_irq(d->irq, d);
    pci_free_irq_vectors(pdev);
    pci_iounmap(pdev, d->mmio);
    pci_release_mem_regions(pdev);
    pci_disable_device(pdev);
}

static const struct pci_device_id pcisign_ids[] = {
    { PCI_DEVICE(PCISIGN_VENDOR, PCISIGN_DEVICE) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, pcisign_ids);

static struct pci_driver pcisign_pci_driver = {
    .name     = "pcisign",
    .id_table = pcisign_ids,
    .probe    = pcisign_probe,
    .remove   = pcisign_remove,
};

module_pci_driver(pcisign_pci_driver);
MODULE_LICENSE("GPL");


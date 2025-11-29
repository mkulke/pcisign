#ifndef __LINUX_PCISIGN_H
#define __LINUX_PCISIGN_H

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/io.h>

long pcisign_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
int pcisign_open(struct inode *ino, struct file *f);

struct pcisign_dev {
    struct pci_dev *pdev;
    void __iomem *mmio;   /* BAR0 */
    int irq;
    struct completion done;
    struct miscdevice miscdev;
};

static const struct file_operations pcisign_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = pcisign_ioctl,
};

#endif /* __LINUX_PCISIGN_H */

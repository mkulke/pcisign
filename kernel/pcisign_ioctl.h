#ifndef __LINUX_PCISIGN_IOCTL_H
#define __LINUX_PCISIGN_IOCTL_H

#include <linux/types.h>

#define PCISIGN_IOC_MAGIC 'P'
#define PCISIGN_IOC_TEST  _IO(PCISIGN_IOC_MAGIC, 0)
#define PCISIGN_IOC_SIGN  _IOWR(PCISIGN_IOC_MAGIC, 1, struct pcisign_req)

struct pcisign_req {
    __u64 src_ptr;
    __u32 src_len;
    __u64 dst_ptr;
    __u32 dst_len;
    __u32 algo;
};

#endif /* __LINUX_PCISIGN_IOCTL_H */

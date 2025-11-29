// pcisign_test.c
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "pcisign_ioctl.h"

int main(void)
{
    unsigned char src_data[256];
    unsigned char dst_data[128];
	struct pcisign_req req = {0};

    const char *dev = "/dev/pcisign";
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, PCISIGN_IOC_TEST) < 0) {
        perror("PCISIGN_IOC_TEST");
        close(fd);
        return 1;
    }

    puts("PCISIGN_IOC_TEST ok");

    memset(src_data, 0x12, sizeof(src_data));
    memset(dst_data, 0   , sizeof(dst_data));
	req.src_ptr = (unsigned long)src_data;
	req.src_len = sizeof(src_data);
	req.dst_ptr = (unsigned long)dst_data;
	req.dst_len = sizeof(dst_data);
	req.algo = 1;

	if (ioctl(fd, PCISIGN_IOC_SIGN, &req) < 0) {
		perror("PCISIGN_IOC_SIGN");
		close(fd);
		return 1;
	}

	/* print signature */
	for (size_t i = 0; i < req.dst_len; i++) {
		printf("%02x", dst_data[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}

    puts("PCISIGN_IOC_SIGN ok");

out:
    close(fd);
    return 0;
}

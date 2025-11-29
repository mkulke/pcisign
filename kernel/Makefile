obj-m := pcisign.o
pcisign-objs := pcisign_drv_main.o pcisign_ioctl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

SRC=.
DST=cloud@localhost:driver
EXCLUDES = \
        --exclude .git \
        --exclude '*.o' \
        --exclude '*.swp'

sync:
	rsync -avz $(EXCLUDES) -e "ssh -p 2222" $(SRC)/ $(DST)

pcisign_test: pcisign_test.c
	$(CC) -o $@ pcisign_test.c

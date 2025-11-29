# pcisign

QEMU backend and kernel driver for pci peripheral that signs payloads

## QEMU device (backend)

### Build

```bash
cd qemu
git clone https://github.com/qemu/qemu.git src
cd src
# known to work revision
git checkout 9ef49528b5
git apply ../0001-hw-misc-pcisign-add-pci-sign-device-backend.patch
# device enabled by default for x86_64-softmmu target
./configure --target-list=x86_64-softmmu ...
make -j$(nproc)
```

### Run

specify `-device pcisign` in your QEMU command line

## Kernel driver (frontend)

### Requirements

Assuming Ubuntu 24.04:

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

### Build

The device should report success in the kernel log when loaded.

```bash
cd kernel
make
insomod pcisign.ko
```

### Test

```bash
cd kernel
gcc -o test_pcisign test_pcisign.c
sudo ./test_pcisign
```

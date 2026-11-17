Archway is a kernel written for loongarch64 architecture exclusively. Other architectures are not planned.
This will take some time...

# Requirements
If you're here you prolly have everything I need.. but if you don't then go get yourself qemu-system-loongarch64, build essentials and whatnot.
Once you're done installing things then go ahead and run `make download-required`

# Virtual memory map
At 0x0000000000001000 - 0x00000003ffffffff [16 GiB (excl. NULL page)] is the userspace executable image.
At 0x0000000400000000 - 0x00000007ffffffff [16 GiB] is the userspace stack.
At 0x0000000800000000 - 0x0000000bffffffff [16 GiB] is the userspace shared libraries images.
At 0x0000000c00000000 - 0x0000000cffffffff [1 GiB] is the userspace TLS area.
At 0x0000000d00000000 - 0x000040067fffffff [63 TiB + 998 GiB] is the userspace brk heap.
At 0x0000400680000000 - 0x00007fffffffffff [63 TiB + 998 GiB] is the userspace mmap area.
At 0x8000000000000000 - 0x8fffffffffffffff [1 EiB] is the uncached, strongly ordered MMIO window.
At 0x9000000000000000 - 0x9fffffffffffffff [1 EiB] is the cached, coherent higher-half direct map.
At 0xffff800000000000 - 0xffff8fffffffffff [16 TiB] is the kernel's heap.
At 0xffff900000000000 - 0xffffffff7fffffff [112 TiB - 2 GiB] is the kernel's vmalloc area.
At 0xffffffff80000000 - 0xffffffff8fffffff [256 MiB] is the kernel image.
At 0xffffffff90000000 - 0xffffffffffffffff [1.75 GiB] is the kernel drivers images area.

plz remember that it wants from cpu VALEN=48
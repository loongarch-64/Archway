Archway is a kernel written for loongarch64 architecture exclusively. Other architectures are not planned.
This will take some time...

# Requirements
If you're here you prolly have everything I need.. but if you don't then go get yourself qemu-system-loongarch64, build essentials and whatnot.
Once you're done installing things then go ahead and run `make download-required`

# Virtual memory map

| Range | Size | Description |
| --- | --- | --- |
| `0x0000000000001000` - `0x00000003ffffffff` | 16 GiB (excl. NULL page) | userspace executable image |
| `0x0000000400000000` - `0x00000007ffffffff` | 16 GiB | userspace stack |
| `0x0000000800000000` - `0x0000000bffffffff` | 16 GiB | userspace shared libraries images |
| `0x0000000c00000000` - `0x0000000cffffffff` | 1 GiB | userspace TLS area |
| `0x0000000d00000000` - `0x000040067fffffff` | 63 TiB + 998 GiB | userspace brk heap |
| `0x0000400680000000` - `0x00007fffffffffff` | 63 TiB + 998 GiB | userspace mmap area |
| `0x8000000000000000` - `0x8fffffffffffffff` | 1 EiB | uncached, strongly ordered MMIO window |
| `0x9000000000000000` - `0x9fffffffffffffff` | 1 EiB | cached, coherent higher-half direct map |
| `0xffff800000000000` - `0xffff8fffffffffff` | 16 TiB | kernel's heap |
| `0xffff900000000000` - `0xffffffff7fffffff` | 112 TiB - 2 GiB | kernel's vmalloc area |
| `0xffffffff80000000` - `0xffffffff8fffffff` | 256 MiB | kernel image |
| `0xffffffff90000000` - `0xffffffffffffffff` | 1.75 GiB | kernel drivers images area |

plz remember that it wants from cpu VALEN=48
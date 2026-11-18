Archway is a kernel written for loongarch64 architecture exclusively. Other architectures are not planned.
This will take some time...

# Requirements
If you're here you prolly have everything I need.. but if you don't then go get yourself qemu-system-loongarch64, build essentials and whatnot.
Once you're done installing things then go ahead and run `make download-required`

# Virtual memory map

| Range | Size | Description |
| --- | --- | --- |
| `0x0000000000001000` - `0x00000003ffffffff` | 16 GiB (excl. NULL page) | userspace executable image |
| `0x0000000400000000` - `0x00007fffffffffff` | 127 TiB + 1008 GiB | userspace mmap area |
| `0x8000000000000000` - `0x8fffffffffffffff` | 1 EiB | uncached, strongly ordered MMIO window |
| `0xffff800000000000` - `0xffffffff7fffffff` | 128 TiB - 2 GiB | kernel dynamic mappings |
| `0xffffffff80000000` - `0xffffffff8fffffff` | 256 MiB | kernel image |
| `0xffffffff90000000` - `0xffffffffffffffff` | 1.75 GiB | kernel drivers images area |

plz remember that it wants from cpu VALEN=48

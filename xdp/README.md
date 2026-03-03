# XDP program notes

The default `zeta_xdp_kern.c` is a **minimal** XDP redirect program that avoids `vmlinux.h`
so it builds easily on most systems.

If you want to extend the program with CO-RE (recommended when you start reading kernel structs),
generate `xdp/vmlinux.h` on your target machine:

```bash
bpftool btf dump file /sys/kernel/btf/vmlinux format c > xdp/vmlinux.h
```

Then include it from your XDP program.

#!/usr/bin/env bash
set -euo pipefail
out="${1:-xdp/vmlinux.h}"
bpftool btf dump file /sys/kernel/btf/vmlinux format c > "${out}"
echo "Wrote ${out}"

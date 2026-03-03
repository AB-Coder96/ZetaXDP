// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
//
// Minimal XDP program that redirects packets to an AF_XDP socket (XSKMAP).
// This file intentionally avoids CO-RE/vmlinux.h to keep the build easy.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
  __uint(type, BPF_MAP_TYPE_XSKMAP);
  __uint(max_entries, 64);   // adjust as needed
  __type(key, __u32);
  __type(value, __u32);
} xsks_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 4);
  __type(key, __u32);
  __type(value, __u64);
} counters SEC(".maps");

static __always_inline void bump(__u32 idx) {
  __u64 *v = bpf_map_lookup_elem(&counters, &idx);
  if (v) *v += 1;
}

SEC("xdp")
int xdp_redirect(struct xdp_md *ctx) {
  __u32 qid = ctx->rx_queue_index;

  bump(0); // seen

  // Redirect to AF_XDP socket bound to qid if present, otherwise pass.
  if (bpf_map_lookup_elem(&xsks_map, &qid)) {
    bump(1); // redirected
    return bpf_redirect_map(&xsks_map, qid, 0);
  }

  bump(2); // passed
  return XDP_PASS;
}

char _license[] SEC("license") = "Dual BSD/GPL";

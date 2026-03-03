# Benchmarking checklist

This is a practical checklist for making your numbers believable and repeatable.

## Before you run

- Build Release (`-DCMAKE_BUILD_TYPE=Release`)
- Disable frequency scaling if possible:
  - set CPU governor to `performance`
  - avoid SMT if you're chasing p99.9 and above (optional)
- Pin threads:
  - receiver `--cpu`
  - generator `--cpu`
- Tune NIC offloads for latency tests (optional):
  - `scripts/nic_tune.sh eth0`

## Run discipline

- Warm up 5–10s (or do a throw-away run).
- Fix payload size and seed.
- Record:
  - kernel version
  - CPU model
  - commit SHA
  - driver + firmware versions (`ethtool -i`)

## Suggested matrix

- socket: {batch 1, 16, 64}
- xdp_copy: {batch 16, 64, 256}
- xdp_zc: same, only if supported

For each: vary ring sizes, then busy-poll/affinity.

## Profiling

- `perf top -p $(pidof zetaxdp)`
- `perf record -F 999 -g -- ./zetaxdp ...`
- `perf report`

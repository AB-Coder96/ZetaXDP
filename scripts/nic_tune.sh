#!/usr/bin/env bash
set -euo pipefail
IFACE="${1:?usage: nic_tune.sh <iface>}"
echo "Disabling GRO/LRO and setting RX/TX coalescing to low-latency defaults on ${IFACE}."
sudo ethtool -K "${IFACE}" gro off lro off tso off gso off || true
sudo ethtool -C "${IFACE}" rx-usecs 0 rx-frames 0 tx-usecs 0 tx-frames 0 || true
sudo ethtool -G "${IFACE}" rx 4096 tx 4096 || true
echo "Done. Verify with: ethtool -k ${IFACE} ; ethtool -c ${IFACE} ; ethtool -g ${IFACE}"

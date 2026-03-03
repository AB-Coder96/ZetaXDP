    # ZetaXDP — Kernel-Bypass UDP Ingest (AF_XDP)

    ZetaXDP is a focused systems project exploring **kernel-bypass UDP ingest** using **AF_XDP**. The goal is to demonstrate the exact performance work HFT teams care about:

    - fast, predictable packet ingest (**pps + tail latency**)
    - correctness under load (**loss detection**, gaps, counters)
    - reproducible benchmarks (**p99/p99.9**, commit tracking)
    - profiling-driven tuning (**perf**, CPU affinity, cache effects)

    **Scope note:** ZetaXDP is a transport/ingest prototype intended for learning and measurement. It is not represented as a production trading connector.

    ## Core constraints

    - **C++20**, userspace ingest loop + generator
    - **CMake**
    - **Linux-only**
    - **libbpf / bpftool** (for XDP program build/loading)
    - **AF_XDP** with UMEM + RX rings

    ## Tooling

    - `clang` (for eBPF/XDP program build)
    - `ip link`, `ethtool`, `tc`, `bpftool`
    - `perf` for profiling

    ## Architecture

    ```
    NIC RX
      ↓
    XDP program (attach to interface)
      ↓
    UMEM frames (copy vs zero-copy)
      ↓
    AF_XDP RX ring → userspace poll loop
      ↓
    Parser (UDP payload decode)
      ↓
    Lock-free ring buffer (optional / stubbed in MVP)
      ↓
    Downstream pipeline (book builder / replay / recorder)
    ```

    ## Modes

    - **Baseline**: kernel UDP socket ingest (comparison point)
    - **AF_XDP copy**: AF_XDP path with copy semantics (portable)
    - **AF_XDP zero-copy**: target path (when supported by driver/NIC)

    ## What it measures

    ### Performance

    - packets/sec (pps)
    - p50/p99/p99.9 ingest loop latency (**RX → parse → commit**) in **nanoseconds**
    - CPU utilization per thread / core
    - cache miss / branch stats (`perf`)

    ### Correctness / loss diagnostics

    - sequence-gap detector (per flow) if payload includes sequence numbers
    - ring saturation / UMEM pressure indicators (fill ring starvation)
    - drop counters (socket drops, XDP/driver drops when available)
    - optional “debug mirror” hooks (future extension)

    ### Reproducible benchmarking discipline

    Every run records:

    - **config hash** (mode, batch, ring sizes, busy-poll, etc.)
    - generator parameters (packet size, rate, flows, seed)
    - compile-time **git SHA** (if built from a git checkout)
    - machine profile (kernel, CPU model, governor hint)

    Recommended run checklist:

    - pin threads to cores (affinity)
    - warm up before measurement
    - fixed payload sizes / fixed generator seeds
    - Release builds only
    - export CSV for dashboards

    ## Quickstart

    ### Dependencies (Ubuntu-ish)

    ```
    sudo apt-get update
    sudo apt-get install -y build-essential cmake clang llvm pkg-config \
      libbpf-dev libelf-dev zlib1g-dev linux-tools-common linux-tools-$(uname -r) \
      bpftool ethtool iproute2
    ```

    > Your distro package names may differ. On some distros you may need to build libbpf from source.

    ### Build

    ```
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j
    ```

    This builds:
    - `zetaxdp` (receiver / benchmark runner)
    - `zetagen` (UDP packet generator)
    - `xdp/zeta_xdp_kern.o` (XDP program object) if `clang` is available

    ### Run (baseline vs AF_XDP)

    Baseline socket ingest:
    ```
    sudo ./zetaxdp --mode socket --iface eth0 --port 9000 --seconds 30 --out out/socket.csv
    ```

    AF_XDP copy mode:
    ```
    sudo ./zetaxdp --mode xdp_copy --iface eth0 --queue 0 --seconds 30 --out out/xdp_copy.csv
    ```

    AF_XDP zero-copy mode (if supported):
    ```
    sudo ./zetaxdp --mode xdp_zc --iface eth0 --queue 0 --seconds 30 --out out/xdp_zc.csv
    ```

    Generator (send to the same host):
    ```
    ./zetagen --dst 127.0.0.1 --port 9000 --seconds 30 --size 128 --rate_pps 2000000 --flows 1
    ```

    ## CSV output

    `zetaxdp` emits a single summary row by default:

    - `measured_pps`
    - `p50_ns`, `p99_ns`, `p999_ns`
    - `rx_pkts`, `gaps`
    - `config_hash`, `git_sha`, `kernel`, `cpu_model`

    You can extend it to emit per-second time series (`--timeseries out.csv`) if desired.

    ## Profiling tips

    - Disable frequency scaling (set `performance` governor) and isolate cores if you can.
    - Pin the RX thread:
      ```
      sudo ./zetaxdp ... --cpu 2
      ```
    - Profile hot path:
      ```
      sudo perf top -p $(pidof zetaxdp)
      sudo perf record -F 999 -g -- ./zetaxdp ...
      sudo perf report
      ```

    ## What to publish on your portfolio page

    Include:

    - pps + p99/p99.9 for socket vs XDP copy vs XDP zero-copy
    - one paragraph on what moved the tail (affinity, batching, ring sizes, busy-poll)
    - a perf screenshot/snippet showing the hot path
    - a “loss dashboard” screenshot proving 0.00% drops under your test load

    ## Roadmap (credible extensions)

    - add AF_XDP TX path for loopback tests
    - add AF_PACKET mmap baseline (another comparison)
    - add an eBPF stats exporter (periodic counters)
    - integrate results into ZetaPulse dashboard
    - add CI perf regression (store last-known-good p99)

    ## Notes

    - Latency metrics in this MVP are **userspace ingest-loop latency** (receive → parse → commit), not end-to-end network latency.
    - Zero-copy support depends on driver/NIC; see kernel docs and `ethtool -i`.

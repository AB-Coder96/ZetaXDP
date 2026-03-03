    #include "zetaxdp/modes.h"
    #include "zetaxdp/payload.h"
    #include "zetaxdp/seq.h"
    #include "zetaxdp/util.h"

    #include <bpf/bpf.h>
    #include <bpf/libbpf.h>
    #include <bpf/xsk.h>

    #include <arpa/inet.h>
    #include <linux/if_link.h>
    #include <poll.h>
    #include <sys/resource.h>
    #include <unistd.h>
    #include <cstring>
    #include <iostream>
    #include <memory>
    #include <vector>
#include <algorithm>

    namespace zetaxdp {

    static bool bump_memlock_rlimit() {
      rlimit r{};
      r.rlim_cur = RLIM_INFINITY;
      r.rlim_max = RLIM_INFINITY;
      return setrlimit(RLIMIT_MEMLOCK, &r) == 0;
    }

    struct XdpHandle {
      // libbpf object/program/link
      bpf_object* obj{nullptr};
      bpf_link* link{nullptr};
      int xsks_map_fd{-1};

      // AF_XDP
      xsk_umem* umem{nullptr};
      xsk_socket* xsk{nullptr};

      xsk_ring_prod fill{};
      xsk_ring_cons comp{};
      xsk_ring_cons rx{};
      xsk_ring_prod tx{}; // unused

      std::vector<uint8_t> umem_buf;
      uint64_t umem_size{0};
      uint32_t frame_size{0};
      uint32_t num_frames{0};

      ~XdpHandle() {
        if (xsk) xsk_socket__delete(xsk);
        if (umem) xsk_umem__delete(umem);
        if (link) bpf_link__destroy(link);
        if (obj) bpf_object__close(obj);
      }
    };

    static bool load_and_attach_xdp(const Options& opt, XdpHandle& h) {
      if (!bump_memlock_rlimit()) {
        std::cerr << "Warning: failed to raise RLIMIT_MEMLOCK (try running as root)\n";
      }

      std::string obj_path = opt.xdp_obj_path;
      if (obj_path.empty()) {
    #ifdef ZETAXDP_DEFAULT_XDP_OBJ_PATH
        obj_path = ZETAXDP_DEFAULT_XDP_OBJ_PATH;
    #else
        obj_path = "xdp/zeta_xdp_kern.o";
    #endif
      }

      libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
      libbpf_set_print([](enum libbpf_print_level lvl, const char* fmt, va_list args) -> int {
        if (lvl == LIBBPF_DEBUG) return 0;
        return vfprintf(stderr, fmt, args);
      });

      h.obj = bpf_object__open_file(obj_path.c_str(), nullptr);
      if (!h.obj) {
        std::cerr << "bpf_object__open_file failed: " << obj_path << "\n";
        return false;
      }
      if (bpf_object__load(h.obj) != 0) {
        std::cerr << "bpf_object__load failed\n";
        return false;
      }

      bpf_program* prog = bpf_object__find_program_by_name(h.obj, "xdp_redirect");
      if (!prog) {
        std::cerr << "Could not find program: xdp_redirect\n";
        return false;
      }

      int ifindex = ifindex_from_name(opt.iface);
      if (ifindex <= 0) {
        std::cerr << "Invalid iface: " << opt.iface << "\n";
        return false;
      }

      // Attach XDP program
      h.link = bpf_program__attach_xdp(prog, ifindex);
      if (!h.link) {
        std::cerr << "bpf_program__attach_xdp failed\n";
        return false;
      }

      // Get xsks map fd
      bpf_map* xsks_map = bpf_object__find_map_by_name(h.obj, "xsks_map");
      if (!xsks_map) {
        std::cerr << "Could not find map: xsks_map\n";
        return false;
      }
      h.xsks_map_fd = bpf_map__fd(xsks_map);
      if (h.xsks_map_fd < 0) {
        std::cerr << "bpf_map__fd failed\n";
        return false;
      }

      return true;
    }

    static bool setup_umem_and_xsk(const Options& opt, XdpHandle& h) {
      h.frame_size = opt.frame_size;
      h.num_frames = opt.umem_frames;
      h.umem_size = uint64_t(h.frame_size) * uint64_t(h.num_frames);
      h.umem_buf.resize(h.umem_size);

      xsk_umem_config ucfg{};
      ucfg.fill_size = opt.fill_ring;
      ucfg.comp_size = opt.fill_ring;
      ucfg.frame_size = h.frame_size;
      ucfg.frame_headroom = 0;
      ucfg.flags = 0;

      if (xsk_umem__create(&h.umem,
                           h.umem_buf.data(),
                           h.umem_size,
                           &h.fill,
                           &h.comp,
                           &ucfg) != 0) {
        std::cerr << "xsk_umem__create failed\n";
        return false;
      }

      xsk_socket_config cfg{};
      cfg.rx_size = opt.rx_ring;
      cfg.tx_size = 0;
      cfg.libbpf_flags = 0;
      cfg.xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST;
      cfg.bind_flags = 0;

      if (opt.mode == Mode::XdpZc) {
        cfg.bind_flags |= XDP_ZEROCOPY;
      } else {
        cfg.bind_flags |= XDP_COPY;
      }

      if (xsk_socket__create(&h.xsk,
                             opt.iface.c_str(),
                             opt.queue,
                             h.umem,
                             &h.rx,
                             &h.tx,
                             &cfg) != 0) {
        std::cerr << "xsk_socket__create failed (driver/NIC may not support mode)\n";
        return false;
      }

      // Put xsk fd into xsks map at key=queue
      int xsk_fd = xsk_socket__fd(h.xsk);
      __u32 key = opt.queue;
      __u32 fd = xsk_fd;
      if (bpf_map_update_elem(h.xsks_map_fd, &key, &fd, 0) != 0) {
        std::cerr << "bpf_map_update_elem(xsks_map) failed: " << std::strerror(errno) << "\n";
        return false;
      }

      \
      // Prime fill ring with UMEM frames.
      const uint32_t to_fill = std::min<uint32_t>(h.num_frames, opt.fill_ring);
      uint32_t idx = 0;
      uint32_t n = xsk_ring_prod__reserve(&h.fill, to_fill, &idx);
      if (n == 0) {
        std::cerr << "xsk_ring_prod__reserve(fill) returned 0
";
        return false;
      }

      for (uint32_t i = 0; i < n; ++i) {
        *xsk_ring_prod__fill_addr(&h.fill, idx + i) = uint64_t(i) * uint64_t(h.frame_size);
      }
      xsk_ring_prod__submit(&h.fill, n);

      return true;
    }

    bool run_xdp_mode(const Options& opt, RunStats& stats) {
      XdpHandle h;

      if (!load_and_attach_xdp(opt, h)) {
        return false;
      }
      if (!setup_umem_and_xsk(opt, h)) {
        return false;
      }

      SeqTracker seq(opt.flows);

      int xsk_fd = xsk_socket__fd(h.xsk);
      pollfd pfd{};
      pfd.fd = xsk_fd;
      pfd.events = POLLIN;

      stats.start_ns = now_ns();
      uint64_t end_target = stats.start_ns + uint64_t(opt.seconds) * 1000000000ull;

      \
      while (now_ns() < end_target) {
        int r = ::poll(&pfd, 1, 100 /*ms*/);
        if (r <= 0) continue;

        uint32_t idx_rx = 0;
        uint32_t rcvd = xsk_ring_cons__peek(&h.rx, opt.batch, &idx_rx);
        if (!rcvd) continue;

        // Cache UMEM addresses so we can recycle them after releasing the RX ring.
        std::vector<uint64_t> addrs;
        addrs.reserve(rcvd);

        for (uint32_t i = 0; i < rcvd; ++i) {
          const xdp_desc* desc = xsk_ring_cons__rx_desc(&h.rx, idx_rx + i);
          const uint64_t addr = xsk_umem__extract_addr(desc->addr);
          const uint32_t len = desc->len;

          addrs.push_back(addr);

          uint8_t* pkt = h.umem_buf.data() + addr;

          const uint64_t t0 = now_ns();

          // For simplicity, assume Ethernet+IPv4(no options)+UDP headers are present.
          // In a real project, you'd validate and parse carefully (VLAN, IPv6, IP options, checksums, etc).
          if (len < (14 + 20 + 8 + sizeof(PayloadHeader))) {
            stats.bad_pkts += 1;
            continue;
          }

          // Skip Ethernet (14), IPv4 (assume 20), UDP (8)
          size_t off = 14 + 20 + 8;
          auto* ph = reinterpret_cast<const PayloadHeader*>(pkt + off);
          seq.observe(ph->flow_id, ph->seq);

          const uint64_t t1 = now_ns();
          stats.ingest_hist.add(t1 - t0);
          stats.rx_pkts += 1;
        }

        xsk_ring_cons__release(&h.rx, rcvd);

        // Refill fill ring with the consumed frames.
        uint32_t idx_f = 0;
        uint32_t reserved = xsk_ring_prod__reserve(&h.fill, rcvd, &idx_f);
        if (reserved == rcvd) {
          for (uint32_t i = 0; i < rcvd; ++i) {
            *xsk_ring_prod__fill_addr(&h.fill, idx_f + i) = addrs[i];
          }
          xsk_ring_prod__submit(&h.fill, reserved);
        } else {
          // If fill ring is full, drop recycling for now (will show up as pressure).
          // A production implementation would retry.
        }
      }

      stats.end_ns = now_ns();

      stats.gaps = seq.total_gaps();
      return true;
    }

    } // namespace zetaxdp

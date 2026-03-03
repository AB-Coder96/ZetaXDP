    #include "zetaxdp/cli.h"
    #include <getopt.h>
    #include <iostream>
    #include <sstream>

    namespace zetaxdp {

    static void usage(const char* argv0) {
      std::cerr
        << "Usage: " << argv0 << " --mode <socket|xdp_copy|xdp_zc> --iface eth0 [options]\n"
        << "Options:\n"
        << "  --mode           socket|xdp_copy|xdp_zc\n"
        << "  --iface          interface name (default eth0)\n"
        << "  --port           UDP port (socket mode)\n"
        << "  --queue          RX queue (XDP mode)\n"
        << "  --seconds        run duration\n"
        << "  --batch          batch size (default 64)\n"
        << "  --cpu            pin RX thread to CPU\n"
        << "  --busy_poll      enable busy polling (socket only, best-effort)\n"
        << "  --flows          expected number of flows (for seq tracking)\n"
        << "  --packet_size    expected UDP payload size (for metadata)\n"
        << "  --out            CSV output path\n"
        << "  --xdp_obj        path to XDP object (default: build output)\n"
        << "  --umem_frames    UMEM frame count\n"
        << "  --frame_size     UMEM frame size\n"
        << "  --rx_ring        RX ring size\n"
        << "  --fill_ring      FILL ring size\n"
        << "\n";
    }

    static bool parse_mode(const std::string& s, Mode& m) {
      if (s == "socket") { m = Mode::Socket; return true; }
      if (s == "xdp_copy") { m = Mode::XdpCopy; return true; }
      if (s == "xdp_zc") { m = Mode::XdpZc; return true; }
      return false;
    }

    std::string Options::config_string() const {
      std::ostringstream oss;
      oss << "mode=" << static_cast<int>(mode)
          << ";iface=" << iface
          << ";port=" << port
          << ";queue=" << queue
          << ";seconds=" << seconds
          << ";batch=" << batch
          << ";cpu=" << cpu
          << ";busy_poll=" << (busy_poll ? 1 : 0)
          << ";umem_frames=" << umem_frames
          << ";frame_size=" << frame_size
          << ";rx_ring=" << rx_ring
          << ";fill_ring=" << fill_ring
          << ";flows=" << flows
          << ";packet_size=" << packet_size;
      return oss.str();
    }

    bool parse_cli(int argc, char** argv, Options& opt) {
      static option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"iface", required_argument, nullptr, 'i'},
        {"port", required_argument, nullptr, 'p'},
        {"queue", required_argument, nullptr, 'q'},
        {"seconds", required_argument, nullptr, 's'},
        {"batch", required_argument, nullptr, 'b'},
        {"cpu", required_argument, nullptr, 'c'},
        {"busy_poll", no_argument, nullptr, 'B'},
        {"flows", required_argument, nullptr, 'f'},
        {"packet_size", required_argument, nullptr, 'S'},
        {"out", required_argument, nullptr, 'o'},
        {"xdp_obj", required_argument, nullptr, 'x'},
        {"umem_frames", required_argument, nullptr, 1001},
        {"frame_size", required_argument, nullptr, 1002},
        {"rx_ring", required_argument, nullptr, 1003},
        {"fill_ring", required_argument, nullptr, 1004},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
      };

      int ch;
      while ((ch = getopt_long(argc, argv, "m:i:p:q:s:b:c:f:S:o:x:hB", long_opts, nullptr)) != -1) {
        switch (ch) {
          case 'm': {
            Mode m;
            if (!parse_mode(optarg, m)) { usage(argv[0]); return false; }
            opt.mode = m;
            break;
          }
          case 'i': opt.iface = optarg; break;
          case 'p': opt.port = static_cast<uint16_t>(std::stoi(optarg)); break;
          case 'q': opt.queue = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 's': opt.seconds = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'b': opt.batch = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'c': opt.cpu = std::stoi(optarg); break;
          case 'B': opt.busy_poll = true; break;
          case 'f': opt.flows = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'S': opt.packet_size = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'o': opt.out = optarg; break;
          case 'x': opt.xdp_obj_path = optarg; break;
          case 1001: opt.umem_frames = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 1002: opt.frame_size = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 1003: opt.rx_ring = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 1004: opt.fill_ring = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'h': usage(argv[0]); return false;
          default: usage(argv[0]); return false;
        }
      }
      return true;
    }

    } // namespace zetaxdp

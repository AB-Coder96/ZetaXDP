    #include "zetaxdp/cli_gen.h"
    #include <getopt.h>
    #include <iostream>

    namespace zetaxdp {

    static void usage(const char* argv0) {
      std::cerr
        << "Usage: " << argv0 << " --dst 127.0.0.1 --port 9000 [options]\n"
        << "Options:\n"
        << "  --dst          destination IPv4/hostname (default 127.0.0.1)\n"
        << "  --port         UDP port\n"
        << "  --seconds      duration\n"
        << "  --size         UDP payload size (>= sizeof(PayloadHeader), default 128)\n"
        << "  --flows        number of flows (round-robin)\n"
        << "  --rate_pps     target packets per second\n"
        << "  --batch        sendmmsg batch size\n"
        << "  --cpu          pin generator thread to CPU\n"
        << "  --seed         deterministic seed (flow/seq init)\n"
        << "  --no_connect   avoid connect() (slower)\n";
    }

    bool parse_cli_gen(int argc, char** argv, GenOptions& opt) {
      static option long_opts[] = {
        {"dst", required_argument, nullptr, 'd'},
        {"port", required_argument, nullptr, 'p'},
        {"seconds", required_argument, nullptr, 's'},
        {"size", required_argument, nullptr, 'S'},
        {"flows", required_argument, nullptr, 'f'},
        {"rate_pps", required_argument, nullptr, 'r'},
        {"batch", required_argument, nullptr, 'b'},
        {"cpu", required_argument, nullptr, 'c'},
        {"seed", required_argument, nullptr, 1001},
        {"no_connect", no_argument, nullptr, 1002},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
      };

      int ch;
      while ((ch = getopt_long(argc, argv, "d:p:s:S:f:r:b:c:h", long_opts, nullptr)) != -1) {
        switch (ch) {
          case 'd': opt.dst = optarg; break;
          case 'p': opt.port = static_cast<uint16_t>(std::stoi(optarg)); break;
          case 's': opt.seconds = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'S': opt.size = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'f': opt.flows = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'r': opt.rate_pps = static_cast<uint64_t>(std::stoull(optarg)); break;
          case 'b': opt.batch = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 'c': opt.cpu = std::stoi(optarg); break;
          case 1001: opt.seed = static_cast<uint32_t>(std::stoul(optarg)); break;
          case 1002: opt.connect = false; break;
          case 'h': usage(argv[0]); return false;
          default: usage(argv[0]); return false;
        }
      }
      return true;
    }

    } // namespace zetaxdp

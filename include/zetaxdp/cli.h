    #pragma once
    #include <cstdint>
    #include <string>

    namespace zetaxdp {

    enum class Mode {
      Socket,
      XdpCopy,
      XdpZc,
    };

    struct Options {
      Mode mode{Mode::Socket};

      std::string iface{"eth0"};
      uint16_t port{9000};
      uint32_t queue{0};
      uint32_t seconds{10};

      // Receiver tuning
      uint32_t batch{64};
      int cpu{-1};
      bool busy_poll{false};

      // AF_XDP tuning
      uint32_t umem_frames{4096};
      uint32_t frame_size{2048};
      uint32_t rx_ring{2048};
      uint32_t fill_ring{2048};

      // Benchmark metadata
      uint32_t flows{1};
      uint32_t packet_size{128};

      std::string out{"out.csv"};
      std::string xdp_obj_path; // optional override

      // For simple run config hashing.
      std::string config_string() const;
    };

    bool parse_cli(int argc, char** argv, Options& opt);

    } // namespace zetaxdp

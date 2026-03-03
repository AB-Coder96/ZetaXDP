    #pragma once
    #include <cstdint>
    #include <string>

    namespace zetaxdp {

    struct GenOptions {
      std::string dst{"127.0.0.1"};
      uint16_t port{9000};
      uint32_t seconds{10};
      uint32_t size{128};
      uint32_t flows{1};
      uint64_t rate_pps{1000000};  // target
      uint32_t batch{64};
      int cpu{-1};
      uint32_t seed{1};
      bool connect{true}; // connect() UDP socket for speed
    };

    bool parse_cli_gen(int argc, char** argv, GenOptions& opt);

    } // namespace zetaxdp

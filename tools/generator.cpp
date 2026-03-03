    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <unistd.h>

    #include <cstring>
    #include <iostream>
    #include <random>
    #include <string>
    #include <vector>

    #include "zetaxdp/cli_gen.h"
    #include "zetaxdp/payload.h"
    #include "zetaxdp/util.h"

    using namespace zetaxdp;

    static bool resolve_dst(const std::string& host, uint16_t port, sockaddr_in& out) {
      addrinfo hints{};
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_DGRAM;
      addrinfo* res = nullptr;
      int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
      if (rc != 0 || !res) return false;
      auto* in = reinterpret_cast<sockaddr_in*>(res->ai_addr);
      out = *in;
      out.sin_port = htons(port);
      freeaddrinfo(res);
      return true;
    }

    int main(int argc, char** argv) {
      GenOptions opt;
      if (!parse_cli_gen(argc, argv, opt)) return 2;

      if (opt.cpu >= 0) pin_thread_to_cpu(opt.cpu);

      if (opt.size < sizeof(PayloadHeader)) {
        std::cerr << "--size must be >= " << sizeof(PayloadHeader) << "\n";
        return 2;
      }

      int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
      if (fd < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
      }

      sockaddr_in dst{};
      if (!resolve_dst(opt.dst, opt.port, dst)) {
        std::cerr << "resolve failed for dst=" << opt.dst << "\n";
        ::close(fd);
        return 1;
      }

      if (opt.connect) {
        if (connect(fd, reinterpret_cast<sockaddr*>(&dst), sizeof(dst)) != 0) {
          std::cerr << "connect() failed: " << std::strerror(errno) << "\n";
          ::close(fd);
          return 1;
        }
      }

      std::vector<std::vector<uint8_t>> bufs(opt.batch, std::vector<uint8_t>(opt.size, 0));
      std::vector<iovec> iovs(opt.batch);
      std::vector<mmsghdr> msgs(opt.batch);

      // Init seq per flow
      const uint32_t flows = std::max<uint32_t>(opt.flows, 1u);
      std::vector<uint64_t> seq(flows, 1);

      std::mt19937 rng(opt.seed);
      // randomize initial sequences slightly if desired
      for (auto& s : seq) s += (rng() & 0xFF);

      for (uint32_t i = 0; i < opt.batch; ++i) {
        iovs[i].iov_base = bufs[i].data();
        iovs[i].iov_len  = bufs[i].size();

        std::memset(&msgs[i], 0, sizeof(mmsghdr));
        msgs[i].msg_hdr.msg_iov = &iovs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;

        if (!opt.connect) {
          msgs[i].msg_hdr.msg_name = &dst;
          msgs[i].msg_hdr.msg_namelen = sizeof(dst);
        }
      }

      const uint64_t start = now_ns();
      const uint64_t end_target = start + uint64_t(opt.seconds) * 1000000000ull;

      // Rate control: simple fixed interval between packets.
      // For very high pps, busy-looping is expected; tune batch and CPU pinning.
      const double interval_ns = (opt.rate_pps > 0) ? (1e9 / double(opt.rate_pps)) : 0.0;
      uint64_t next_send_ns = now_ns();

      uint64_t sent = 0;

      while (now_ns() < end_target) {
        // Fill batch payloads
        for (uint32_t i = 0; i < opt.batch; ++i) {
          uint32_t flow = (flows == 1) ? 0u : uint32_t(sent + i) % flows;
          auto* ph = reinterpret_cast<PayloadHeader*>(bufs[i].data());
          ph->seq = seq[flow]++;
          ph->tx_ts_ns = now_ns(); // not used by MVP receiver latency, but useful for extensions
          ph->flow_id = flow;
          ph->reserved = 0;
        }

        int rc = sendmmsg(fd, msgs.data(), opt.batch, 0);
        if (rc < 0) {
          if (errno == EINTR) continue;
          std::cerr << "sendmmsg failed: " << std::strerror(errno) << "\n";
          break;
        }
        sent += uint64_t(rc);

        if (interval_ns > 0.0) {
          next_send_ns += uint64_t(interval_ns * double(rc));
          while (now_ns() < next_send_ns) {
            // busy wait
          }
        }
      }

      const uint64_t end = now_ns();
      double secs = double(end - start) / 1e9;
      std::cout << "sent=" << sent << " secs=" << secs << " pps=" << (secs > 0 ? (double(sent)/secs) : 0.0) << "\n";

      ::close(fd);
      return 0;
    }

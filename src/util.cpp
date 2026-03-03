    #include "zetaxdp/util.h"
    #include <chrono>
    #include <cstring>
    #include <fstream>
    #include <iostream>
    #include <optional>
    #include <sched.h>
    #include <sys/stat.h>
    #include <sys/utsname.h>
    #include <net/if.h>

    namespace zetaxdp {

    uint64_t now_ns() {
      // Monotonic raw is stable for profiling/latency in a single machine.
      timespec ts{};
      clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
      return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
    }

    std::string uname_string() {
      utsname u{};
      if (uname(&u) != 0) return "unknown";
      return std::string(u.sysname) + " " + u.release;
    }

    std::string cpu_model_string() {
      std::ifstream in("/proc/cpuinfo");
      if (!in) return "unknown";
      std::string line;
      while (std::getline(in, line)) {
        if (line.rfind("model name", 0) == 0) {
          auto pos = line.find(':');
          if (pos != std::string::npos) {
            return line.substr(pos + 1);
          }
        }
      }
      return "unknown";
    }

    bool pin_thread_to_cpu(int cpu) {
      cpu_set_t set;
      CPU_ZERO(&set);
      CPU_SET(cpu, &set);
      if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        std::cerr << "sched_setaffinity failed: " << std::strerror(errno) << "\n";
        return false;
      }
      return true;
    }

    uint64_t fnv1a64(const void* data, size_t n, uint64_t seed) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
      uint64_t h = seed;
      for (size_t i = 0; i < n; ++i) {
        h ^= uint64_t(p[i]);
        h *= 1099511628211ull;
      }
      return h;
    }

    uint64_t fnv1a64_str(const std::string& s, uint64_t seed) {
      return fnv1a64(s.data(), s.size(), seed);
    }

    int ifindex_from_name(const std::string& ifname) {
      return if_nametoindex(ifname.c_str());
    }

    bool ensure_parent_dir(const std::string& path) {
      auto slash = path.find_last_of('/');
      if (slash == std::string::npos) return true;
      std::string dir = path.substr(0, slash);
      if (dir.empty()) return true;

      struct stat st{};
      if (stat(dir.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
      }
      // naive single-level mkdir; users can pre-create nested if needed
      if (mkdir(dir.c_str(), 0755) != 0) {
        if (errno == EEXIST) return true;
        return false;
      }
      return true;
    }

    } // namespace zetaxdp

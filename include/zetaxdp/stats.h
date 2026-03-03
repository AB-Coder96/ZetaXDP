    #pragma once
    #include <cstdint>
    #include <vector>
    #include <string>

    namespace zetaxdp {

    // A small log2 histogram for nanosecond latencies.
    // Buckets represent [2^i, 2^(i+1)) ns, with bucket 0 representing [0,1) ns.
    class Log2Histogram {
    public:
      explicit Log2Histogram(uint32_t max_bucket = 63);

      void add(uint64_t value_ns);
      uint64_t count() const { return count_; }

      // Quantile in [0,1]. Returns an upper-bound estimate (bucket high end).
      uint64_t quantile(double q) const;

      std::string debug_summary() const;

    private:
      uint32_t max_bucket_;
      std::vector<uint64_t> buckets_;
      uint64_t count_{0};
    };

    struct RunStats {
      uint64_t rx_pkts{0};
      uint64_t gaps{0};
      uint64_t bad_pkts{0};
      uint64_t start_ns{0};
      uint64_t end_ns{0};

      Log2Histogram ingest_hist;

      RunStats() : ingest_hist(63) {}

      double seconds() const;
      double measured_pps() const;

      uint64_t p50_ns() const { return ingest_hist.quantile(0.50); }
      uint64_t p99_ns() const { return ingest_hist.quantile(0.99); }
      uint64_t p999_ns() const { return ingest_hist.quantile(0.999); }
    };

    } // namespace zetaxdp

    #pragma once
    #include <cstdint>
    #include <vector>

    namespace zetaxdp {

    // Simple per-flow sequence gap detector.
    // Assumes sequences increment by 1.
    class SeqTracker {
    public:
      explicit SeqTracker(uint32_t flows);

      // returns number of gaps detected by this update (0 or 1 typically)
      uint64_t observe(uint32_t flow_id, uint64_t seq);

      uint64_t total_gaps() const { return gaps_; }

    private:
      std::vector<uint64_t> last_;
      uint64_t gaps_{0};
    };

    } // namespace zetaxdp

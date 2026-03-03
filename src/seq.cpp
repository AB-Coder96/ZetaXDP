    #include "zetaxdp/seq.h"
    #include <algorithm>

    namespace zetaxdp {

    SeqTracker::SeqTracker(uint32_t flows)
      : last_(std::max<uint32_t>(flows, 1u), 0) {}

    uint64_t SeqTracker::observe(uint32_t flow_id, uint64_t seq) {
      if (last_.empty()) return 0;
      flow_id %= static_cast<uint32_t>(last_.size());
      uint64_t& last = last_[flow_id];
      if (last != 0 && seq != last + 1) {
        gaps_ += 1;
      }
      last = seq;
      return gaps_;
    }

    } // namespace zetaxdp

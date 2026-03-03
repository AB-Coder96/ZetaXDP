    #pragma once
    #include <cstdint>

    namespace zetaxdp {

    // Payload header embedded in UDP payload.
    // Generator writes seq + tx timestamp; receiver can use seq for loss diagnostics.
    struct PayloadHeader {
      uint64_t seq;
      uint64_t tx_ts_ns;
      uint32_t flow_id;
      uint32_t reserved;
    };

    static_assert(sizeof(PayloadHeader) == 24);

    } // namespace zetaxdp

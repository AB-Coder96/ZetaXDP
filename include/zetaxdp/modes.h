    #pragma once
    #include "zetaxdp/cli.h"
    #include "zetaxdp/stats.h"

    namespace zetaxdp {

    // Runs the selected receiver mode and fills RunStats.
    // Returns true on success.
    bool run_socket_mode(const Options& opt, RunStats& stats);
    bool run_xdp_mode(const Options& opt, RunStats& stats);

    } // namespace zetaxdp

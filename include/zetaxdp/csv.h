    #pragma once
    #include <string>
    #include <unordered_map>
    #include "zetaxdp/stats.h"

    namespace zetaxdp {

    struct CsvRow {
      std::unordered_map<std::string, std::string> kv;
    };

    std::string csv_header();
    std::string csv_row(const CsvRow& row);
    bool write_csv_summary(const std::string& path, const CsvRow& row, bool write_header_if_new = true);

    } // namespace zetaxdp

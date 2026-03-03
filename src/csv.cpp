    #include "zetaxdp/csv.h"
    #include <fstream>
    #include <sstream>
    #include <sys/stat.h>

    namespace zetaxdp {

    static const char* kCols[] = {
      "mode","iface","queue","port","seconds","batch","busy_poll",
      "flows","packet_size",
      "rx_pkts","gaps","bad_pkts",
      "measured_pps",
      "p50_ns","p99_ns","p999_ns",
      "config_hash","git_sha","kernel","cpu_model"
    };

    std::string csv_header() {
      std::ostringstream oss;
      for (size_t i = 0; i < sizeof(kCols)/sizeof(kCols[0]); ++i) {
        if (i) oss << ",";
        oss << kCols[i];
      }
      return oss.str();
    }

    static std::string escape_csv(const std::string& s) {
      bool need = false;
      for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') { need = true; break; }
      }
      if (!need) return s;
      std::ostringstream oss;
      oss << '"';
      for (char c : s) {
        if (c == '"') oss << "\"\"";
        else oss << c;
      }
      oss << '"';
      return oss.str();
    }

    std::string csv_row(const CsvRow& row) {
      std::ostringstream oss;
      for (size_t i = 0; i < sizeof(kCols)/sizeof(kCols[0]); ++i) {
        if (i) oss << ",";
        auto it = row.kv.find(kCols[i]);
        if (it != row.kv.end()) oss << escape_csv(it->second);
      }
      return oss.str();
    }

    static bool file_exists(const std::string& path) {
      struct stat st{};
      return stat(path.c_str(), &st) == 0;
    }

    bool write_csv_summary(const std::string& path, const CsvRow& row, bool write_header_if_new) {
      const bool exists = file_exists(path);
      std::ofstream out(path, std::ios::app);
      if (!out) return false;
      if (write_header_if_new && !exists) {
        out << csv_header() << "\n";
      }
      out << csv_row(row) << "\n";
      return true;
    }

    } // namespace zetaxdp

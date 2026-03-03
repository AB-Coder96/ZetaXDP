    #pragma once
    #include <cstdint>
    #include <string>
    #include <vector>

    namespace zetaxdp {

    uint64_t now_ns();

    std::string uname_string();
    std::string cpu_model_string();

    // Best-effort: set affinity of current thread.
    bool pin_thread_to_cpu(int cpu);

    // FNV-1a 64-bit hash for reproducible config hashing.
    uint64_t fnv1a64(const void* data, size_t n, uint64_t seed = 14695981039346656037ull);
    uint64_t fnv1a64_str(const std::string& s, uint64_t seed = 14695981039346656037ull);

    // Parse "eth0" -> ifindex.
    int ifindex_from_name(const std::string& ifname);

    // Ensure directory exists for output file.
    bool ensure_parent_dir(const std::string& path);

    } // namespace zetaxdp

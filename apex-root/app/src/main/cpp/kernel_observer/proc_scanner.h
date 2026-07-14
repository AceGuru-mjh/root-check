#ifndef APEX_ROOT_PROC_SCANNER_H
#define APEX_ROOT_PROC_SCANNER_H

#include <cstdint>
#include <vector>
#include <string>

namespace apex {
namespace observer {

struct ProcEntry {
    int pid;
    char name[64];
    char state;
    uint64_t flags;
};

struct MemoryRegion {
    uint64_t start;
    uint64_t end;
    char perms[5];
    uint64_t offset;
    char path[256];
};

struct SyscallTrace {
    int64_t syscall_nr;
    uint64_t timestamp_ns;
    int64_t duration_ns;
    int64_t ret_value;
};

std::vector<ProcEntry> scan_processes();
std::vector<MemoryRegion> read_process_maps(int pid);
bool detect_hidden_process(const std::vector<ProcEntry>& baseline);
bool check_memory_integrity(const char* path, const uint8_t* expected_hash);

// Syscall collection
void start_syscall_trace();
SyscallTrace collect_syscall_stats(int64_t syscall_nr);

} // namespace observer
} // namespace apex

#endif // APEX_ROOT_PROC_SCANNER_H

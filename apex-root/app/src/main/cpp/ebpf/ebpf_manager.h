#ifndef APEX_ROOT_EBPF_MANAGER_H
#define APEX_ROOT_EBPF_MANAGER_H

#include <cstdint>
#include <cstddef>

namespace apex {
namespace ebpf {

// eBPF program types
enum class BpfProgType {
    KPROBE,
    TRACEPOINT,
    RAW_TRACEPOINT,
    TP_BTF,
    PERF_EVENT
};

struct BpfProgram {
    int fd;
    int prog_type;
    const char* name;
    bool attached;
};

// Load and attach eBPF programs
int load_bpf_program(const char* path, BpfProgType type);
bool attach_kprobe(int prog_fd, const char* symbol);
bool attach_tracepoint(int prog_fd, const char* category, const char* event);
bool detach_program(int prog_fd);

// Hide operations
bool hide_process(const char* proc_name);
bool hide_file(const char* path);
bool hide_kernel_module(const char* mod_name);
bool protect_syscall_table();
bool spoof_hardware_id(const char* id_type, const char* fake_value);

// Game mode
bool activate_game_mode();
bool deactivate_game_mode();

// Check eBPF subsystem availability
bool check_ebpf_available();

} // namespace ebpf
} // namespace apex

#endif

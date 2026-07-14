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

// ─── Runtime capability / fallback detection ───────────────
//
//  在 Android 13+ GKI 内核 + SELinux enforcing 模式下,普通 root 用户态
//  进程加载 eBPF 程序通常会失败 (需要 bpf { prog_load prog_run } 特权)。
//  此前 check_ebpf_available() 只检查 BPF syscall 是否可用,但即使
//  syscall 可用,SELinux 仍可能拒绝具体操作。
//
//  新增 EbpfCapability 结构 + detect_ebpf_capabilities() 在加载任何
//  eBPF 程序前完整评估环境,让上层根据结果决定走 eBPF 路径还是 fallback
//  (mount namespace + LD_PRELOAD)。
struct EbpfCapability {
    bool bpf_syscall_present;     // BPF syscall 不被 seccomp 拦截
    bool bpffs_mounted;           // /sys/fs/bpf 已挂载
    bool kernel_version_ok;       // 内核 >= 5.10 (Android 13 GKI 基线)
    bool selinux_allows_bpf;      // SELinux 不处于 enforcing 拒绝 bpf 类
    bool can_create_map;          // 实测能否 BPF_MAP_CREATE (真实权限探测)
    bool can_load_prog;           // 实测能否 BPF_PROG_LOAD (真实权限探测)
    int  kernel_major;            // 内核主版本号
    int  kernel_minor;            // 内核次版本号
    char selinux_mode[16];        // "enforcing" / "permissive" / "disabled" / "unknown"
};

// 探测当前进程的 eBPF 能力。该函数本身不会加载任何 kprobe/tracepoint,
// 仅做 BPF_MAP_CREATE + BPF_PROG_LOAD 的最小可行测试并立即关闭 fd,
// 因此对系统无副作用。
//
// 返回值始终为 true;详细信息填入 *out_cap。
// 若 out_cap 为 nullptr,仅返回 true (用于简单可用性判断)。
bool detect_ebpf_capabilities(EbpfCapability* out_cap);

// 根据 detect_ebpf_capabilities() 的结果判断是否应启用 eBPF 路径。
// 上层 (guard_engine / hide_mode) 调用此函数决定走 eBPF 还是 fallback。
bool should_use_ebpf_path();

} // namespace ebpf
} // namespace apex

#endif

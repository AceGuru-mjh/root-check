// v1.1.1 修复 P0-C4: namespace_isolation 重写为安全 stub
// ─────────────────────────────────────────────────────────────
// 此前实现存在严重错误 (详见审计 worklog P0-C4):
//   1. pipe2 第一个参数传 0 (应该传 int[2] 数组指针) — 永远返回 -EFAULT
//   2. mmap flags 传 0x20 (MAP_ANONYMOUS) 但缺 MAP_SHARED (0x01)
//      → 父子进程映射不同物理页, seccomp 信号机制完全失效
//   3. PTRACE_DETACH (0x4200) 的 asm 块丢失 pid 操作数 (x0 = 0x4200 而非 pid)
//      → 实际 ptrace 第一个参数是 request 而不是 pid, 语义彻底反了
//   4. mount_pure_system 用 openat(O_RDONLY|O_CREAT) 想建目录 — openat 不能
//      建目录, 应该用 mkdirat(AT_FDCWD, path, 0755)
//   5. clone(CLONE_NEWNS|CLONE_NEWPID|...) 在普通 app (uid != 0) 上下文
//      必然 EPERM — 需要 CAP_SYS_ADMIN
//   6. overlayfs mount 同样需 CAP_SYS_ADMIN, 且 Android SELinux 默认禁止
//      mount overlay 到任意路径
//
// 这些操作在普通 app (无 root) 上下文下根本不可能成功。隐藏模式 (hide-mode)
// 在非 root 设备上本就不工作。即便设备已 root, 当前实现仍存在上述 6 个 bug,
// 继续执行只会产生 EPERM/EFAlT/EINVAL 噪音日志, 不会真正隔离任何东西。
//
// 当前实现改为安全 stub:
//   - 所有函数检测 getuid()==0, 非 root 直接返回失败 (-1/false) 并 log
//   - 即使是 root, 也不执行危险的 namespace/mount 操作 (实现本身有 bug)
//   - 函数签名严格保持与 namespace_isolation.h 一致, 不破坏 JNI 调用方
//
// 真正的 namespace 隔离将在 v1.2.0 用正确方式重写:
//   - 通过 root daemon (trusted_daemon) 执行 unshare/mount, daemon 用
//     正确的 pipe2(int[2], O_CLOEXEC) + mmap(..., MAP_SHARED|MAP_ANONYMOUS)
//   - 或通过 Shizuku API 委托特权操作 (无需 app 自己持 root)
// ─────────────────────────────────────────────────────────────

#include "namespace_isolation.h"
#include "../common/syscall.h"
#include <android/log.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>

// <android/log.h> 已定义 ANDROID_LOG_WARN / ANDROID_LOG_INFO, 无需重新定义。
#define ISLAND_TAG "ApexIsland"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, ISLAND_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  ISLAND_TAG, __VA_ARGS__)

namespace apex {
namespace island {

namespace {
// 检测当前进程是否以 root (uid == 0) 运行。
// 注意: 即使返回 true, 当前实现仍不执行 namespace 操作 (见文件顶部注释)。
bool is_root_uid() {
    return getuid() == 0;
}
} // namespace

int create_isolated_environment(const char* sandbox_name) {
    (void)sandbox_name;  // stub 未使用

    // 非 root 设备无法创建 namespace (CLONE_NEWNS / CLONE_NEWPID 均需 CAP_SYS_ADMIN)
    if (!is_root_uid()) {
        LOGW("create_isolated_environment: requires root (CAP_SYS_ADMIN), "
             "current uid=%d. Hide mode unavailable on non-rooted devices.",
             getuid());
        return -1;
    }

    // 即使是 root, 当前实现仍有 bug (pipe2/mmap/ptrace/clone 全部错误), 暂不执行。
    // v1.2.0 将通过 trusted_daemon 用正确的 syscall 参数重新实现。
    LOGW("create_isolated_environment: root detected but implementation is stub. "
         "Namespace isolation will be available in v1.2.0.");
    return -1;
}

bool destroy_isolated_environment(int pid) {
    (void)pid;  // stub: 没有创建就没有销毁
    // create_isolated_environment 始终返回 -1, 所以没有任何 pid 需要销毁。
    // 保留函数以维持 ABI, 真正实现见 v1.2.0。
    LOGI("destroy_isolated_environment: stub (no-op, pid=%d)", pid);
    return false;
}

bool run_in_environment(int pid, const char* cmd) {
    (void)pid;
    (void)cmd;
    // setns 进入子进程的 PID namespace 也需要 CAP_SYS_ADMIN,
    // 且 create_isolated_environment 从未真正创建过隔离环境。
    LOGI("run_in_environment: stub (no-op, pid=%d)", pid);
    return false;
}

bool apply_seccomp_bpf(int pid) {
    (void)pid;
    // 原 apply_seccomp_bpf 实现:
    //   - 通过 /proc/pid/mem 写入 mmap 标志, 但 mmap 缺 MAP_SHARED → 标志写入
    //     的是父进程自己的物理页, 子进程永远看不到
    //   - 退化到 ptrace 注入, 但 PTRACE_DETACH asm 丢失 pid 操作数 → detach
    //     不到正确的子进程
    // 即使有 root, seccomp(2) 也需要 CAP_SYS_ADMIN 或 NO_NEW_PRIVS 已设置。
    // 当前实现为 stub, v1.2.0 将通过 trusted_daemon 用 PR_SET_SECCOMP 正确注入。
    LOGI("apply_seccomp_bpf: stub (no-op, pid=%d)", pid);
    return false;
}

bool mount_pure_system(const char* sandbox_root) {
    if (!sandbox_root) return false;

    if (!is_root_uid()) {
        LOGW("mount_pure_system: requires root, current uid=%d", getuid());
        return false;
    }

    // 原 mount_pure_system 实现:
    //   - mkdir_p 用 openat(O_RDONLY|O_CREAT) — openat 不能建目录, 必须用
    //     mkdirat(AT_FDCWD, path, 0755)
    //   - overlayfs mount 需要 CAP_SYS_ADMIN, 且 Android SELinux 默认禁止
    //     mount("overlay", ...) 到非 /data/local/tmp 等白名单路径
    // stub: v1.2.0 将用 mkdirat + 正确的 mount 调用重写, 并通过 trusted_daemon
    // 执行, 而不是在 app 进程内直接做。
    LOGW("mount_pure_system: root detected but implementation is stub (mkdir "
         "uses openat incorrectly, overlayfs mount blocked by SELinux). "
         "Will be rewritten in v1.2.0.");
    return false;
}

} // namespace island
} // namespace apex

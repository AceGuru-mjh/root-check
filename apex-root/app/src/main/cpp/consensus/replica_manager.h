#ifndef APEX_ROOT_REPLICA_MANAGER_H
#define APEX_ROOT_REPLICA_MANAGER_H

#include <cstdint>
#include <functional>
#include <array>
#include <vector>

// ─────────────────────────────────────────────────────────────
// P0-D6 注释 (v1.1.1): 本模块为实验性死代码 (experimental / dead code)
// ----------------------------------------------------------------
// 当前状态:
//   - CMakeLists.txt 仍编译 consensus/*.cpp, 符号被链接进 libapex_root.so
//   - 但没有任何 JNI 入口暴露这些函数给 Kotlin 层
//   - 也没有任何 native 主流程 (runQuickScan / service_engine::execute_scan) 调用它们
//   - 因此 replica_manager 的代码在运行时永远不会被执行
//
// 原始设计意图 (未实现):
//   - 三副本共识 (Replica A/B/C) 跨命名空间运行, 用后量子签名互验检测结果
//   - Replica B/C 通过 fork + unshare(CLONE_NEWPID|CLONE_NEWNS) 创建
//
// 为何停用:
//   1. P0-C4: namespace_isolation.cpp 实现错误 (pipe2/mmap/ptrace/mkdir 全部
//      用错), 依赖它的 start_replica() 不可能成功
//   2. P0-C5: 非 aarch64 平台 syscall 失效, fork 路径走不通
//   3. 在 JVM 进程内 fork 长生命周期的子进程会触发 Android LowMemoryKiller
//      并破坏 Zygote 的进程模型
//
// 重启计划: v1.2.0 将通过独立的 trusted_daemon (root 进程, 非 JVM 子进程)
// 重新实现三副本共识, 并通过 binder/socket 暴露给 app。
//
// 本头文件保留供未来重写时参考。**当前不要在 native-lib.cpp 中添加任何
// 调用本模块的 JNI 入口** — 这样会激活有 bug 的代码路径。
// ─────────────────────────────────────────────────────────────

namespace apex {
namespace consensus {

enum class ReplicaRole {
    REPLICA_A,  // Main process namespace
    REPLICA_B,  // Forked mnt/pid namespace
    REPLICA_C   // chroot sandbox
};

enum class ReplicaState {
    RUNNING,
    SUSPECTED,
    FAILED,
    RESTARTING
};

struct ReplicaStatus {
    ReplicaRole role;
    ReplicaState state;
    int pid;
    uint64_t heartbeat_ns;
    uint32_t failure_count;
    uint8_t code_hash[64];
};

// Post-quantum signed detection result for consensus
struct SignedVote {
    uint8_t result_hash[64];
    ReplicaRole from;
    std::vector<uint8_t> dilithium_signature;
    std::vector<uint8_t> public_key;
    uint64_t timestamp_ns;
};

namespace replica_manager {

bool start_replica(ReplicaRole role, bool with_isolation);
bool stop_replica(ReplicaRole role);
ReplicaStatus get_status(ReplicaRole role);
bool monitor_replicas();
bool restart_failed_replica(ReplicaRole role);

// Consensus voting (post-quantum signed)
bool submit_vote(const uint8_t* result_hash, ReplicaRole from);
bool submit_signed_vote(const SignedVote& vote);
bool has_consensus(const uint8_t* result_hash);
int get_consensus_count(const uint8_t* result_hash);
bool verify_vote_signature(const SignedVote& vote);
uint64_t get_current_round();
bool run_consensus_round(const uint8_t* result_hash);
bool update_heartbeat(ReplicaRole role);

// Post-quantum key management per replica
bool init_replica_keys();
std::vector<uint8_t> get_replica_public_key(ReplicaRole role);

} // namespace replica_manager
} // namespace consensus
} // namespace apex

#endif // APEX_ROOT_REPLICA_MANAGER_H

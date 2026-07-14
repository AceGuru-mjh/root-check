#ifndef APEX_ROOT_REPLICA_MANAGER_H
#define APEX_ROOT_REPLICA_MANAGER_H

#include <cstdint>
#include <functional>
#include <array>
#include <vector>

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

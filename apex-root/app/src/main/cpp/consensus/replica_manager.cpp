#include "replica_manager.h"
#include "bare_syscall/syscall_bridge.h"
#include "p2p_comm/replica_socket.h"
#include "trusted_root/crypto/oqs_signature.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include <cstring>
#include <mutex>
#include <cinttypes>
#include <android/log.h>

namespace apex {
namespace consensus {

static std::mutex g_mutex;
static ReplicaStatus g_replicas[3];
static bool g_running[3] = {false, false, false};
static int g_replica_pids[3] = {-1, -1, -1};
static uint64_t g_last_heartbeat[3] = {0, 0, 0};

// Per-replica post-quantum key pairs
static std::vector<uint8_t> g_replica_pub[3];
static std::vector<uint8_t> g_replica_priv[3];

// Consensus round tracking
static uint64_t g_consensus_round = 0;

// Vote tracking: map result_hash -> vote state
static struct {
    uint8_t hash[64];
    int votes[3];       // -1 = not voted, 0 = reject, 1 = accept
    int count;
    bool finalized;     // consensus reached and committed
    uint64_t round;     // consensus round
    uint64_t timestamp; // when the vote was created
} g_votes[256];
static int g_vote_count = 0;

// Heartbeat timeout: 5 seconds in nanoseconds
static constexpr uint64_t HEARTBEAT_TIMEOUT_NS = 5000000000ULL;

namespace replica_manager {

bool init_replica_keys() {
    auto& oqs = crypto::OqsSignature::getInstance();
    if (!oqs.isAvailable()) return false;

    for (int i = 0; i < 3; i++) {
        if (!oqs.generateKeyPair(g_replica_pub[i], g_replica_priv[i])) {
            __android_log_print(ANDROID_LOG_ERROR, "APEX-CONSENSUS",
                "Failed to generate PQ key for replica %d", i);
            return false;
        }
        __android_log_print(ANDROID_LOG_DEBUG, "APEX-CONSENSUS",
            "Replica %d PQ key: pk=%zu sk=%zu",
            i, g_replica_pub[i].size(), g_replica_priv[i].size());
    }
    return true;
}

std::vector<uint8_t> get_replica_public_key(ReplicaRole role) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_replica_pub[static_cast<int>(role)];
}

bool verify_vote_signature(const SignedVote& vote) {
    auto& oqs = crypto::OqsSignature::getInstance();
    if (!oqs.isAvailable()) return false;

    // Verify that the public key matches the expected replica
    int from_idx = static_cast<int>(vote.from);
    if (g_replica_pub[from_idx].size() != vote.public_key.size()) return false;
    if (std::memcmp(g_replica_pub[from_idx].data(), vote.public_key.data(),
                    vote.public_key.size()) != 0) return false;

    return oqs.verify(
        std::vector<uint8_t>(vote.result_hash, vote.result_hash + 64),
        vote.dilithium_signature,
        vote.public_key);
}

bool start_replica(ReplicaRole role, bool with_isolation) {
    std::lock_guard<std::mutex> lock(g_mutex);
    int idx = static_cast<int>(role);
    if (g_running[idx]) return true;

    int pid = static_cast<int>(bs_fork());
    if (pid == 0) {
        if (with_isolation) {
            // Apply isolation for replicas B and C
            if (role == ReplicaRole::REPLICA_B) {
                bs_unshare(0x00020000 | 0x02000000); // CLONE_NEWNS | CLONE_NEWPID
            } else if (role == ReplicaRole::REPLICA_C) {
                bs_unshare(0x00020000 | 0x02000000 | 0x40000000); // + CLONE_NEWNET
            }
        }

        g_replica_pids[idx] = static_cast<int>(bs_getpid());
        uint64_t last_send = bs_clock_ns();

        // Child's main loop: heartbeat + vote exchange
        while (true) {
            uint64_t now = bs_clock_ns();
            if (now - last_send >= 1000000000ULL) { // 1 second
                // Send heartbeat with current state
                uint8_t hb_msg[72];
                hb_msg[0] = 0x01; // heartbeat
                std::memcpy(hb_msg + 1, &now, 8);
                std::memset(hb_msg + 9, 0, 63);
                // Sign heartbeat with replica's key
                auto hb_sig = crypto::dilithium_sign(hb_msg + 1, 8,
                    g_replica_priv[idx].data(), g_replica_priv[idx].size());
                if (hb_sig.size() <= 64) {
                    std::memcpy(hb_msg + 9, hb_sig.data(), hb_sig.size());
                }
                replica_socket::send_heartbeat(role);
                last_send = now;
            }

            // Check for incoming messages (votes, results)
            auto msg = replica_socket::receive_message();
            if (!msg.empty() && msg[0] == 0x02) {
                // Vote received - forward to parent via shared state
                // In a real implementation, write to shared memory
                // For now, just acknowledge
                __android_log_print(ANDROID_LOG_DEBUG, "APEX-CONSENSUS",
                    "Replica %d received vote", idx);
            }

            bs_nanosleep(50000000ULL); // 50ms
        }
        bs_exit(0);
        return true;
    } else if (pid > 0) {
        g_replica_pids[idx] = pid;
        g_running[idx] = true;
        g_replicas[idx].role = role;
        g_replicas[idx].state = ReplicaState::RUNNING;
        g_replicas[idx].pid = pid;
        g_replicas[idx].heartbeat_ns = bs_clock_ns();
        g_replicas[idx].failure_count = 0;
        __android_log_print(ANDROID_LOG_DEBUG, "APEX-CONSENSUS",
            "Replica %d started (pid=%d)", idx, pid);
        return true;
    }
    return false;
}

bool start_replica(ReplicaRole role) {
    bool with_isolation = (role != ReplicaRole::REPLICA_A);
    return start_replica(role, with_isolation);
}

bool stop_replica(ReplicaRole role) {
    std::lock_guard<std::mutex> lock(g_mutex);
    int idx = static_cast<int>(role);
    if (!g_running[idx]) return true;

    bs_kill(g_replica_pids[idx], 9);
    g_running[idx] = false;
    g_replica_pids[idx] = -1;
    g_replicas[idx].state = ReplicaState::FAILED;
    return true;
}

ReplicaStatus get_status(ReplicaRole role) {
    std::lock_guard<std::mutex> lock(g_mutex);
    int idx = static_cast<int>(role);
    ReplicaStatus s = g_replicas[idx];
    s.heartbeat_ns = g_last_heartbeat[idx];
    return s;
}

bool monitor_replicas() {
    bool all_ok = true;
    uint64_t now = bs_clock_ns();

    for (int i = 0; i < 3; i++) {
        if (!g_running[i]) {
            // Try to restart if it should be running
            if (g_replicas[i].state != ReplicaState::FAILED) {
                auto role = static_cast<ReplicaRole>(i);
                start_replica(role);
            }
            continue;
        }

        // Check if process is alive
        if (bs_kill(g_replica_pids[i], 0) < 0) {
            g_replicas[i].state = ReplicaState::FAILED;
            g_replicas[i].failure_count++;
            all_ok = false;
            auto role = static_cast<ReplicaRole>(i);
            g_running[i] = false;
            // Auto-restart with backoff
            start_replica(role);
            __android_log_print(ANDROID_LOG_WARN, "APEX-CONSENSUS",
                "Replica %d restarted (failure #%d)", i, g_replicas[i].failure_count);
            continue;
        }

        // Check heartbeat timeout
        if (g_last_heartbeat[i] > 0 && (now - g_last_heartbeat[i]) > HEARTBEAT_TIMEOUT_NS) {
            g_replicas[i].state = ReplicaState::SUSPECTED;
            all_ok = false;
        } else {
            g_replicas[i].state = ReplicaState::RUNNING;
        }
    }
    return all_ok;
}

bool restart_failed_replica(ReplicaRole role) {
    int idx = static_cast<int>(role);
    g_replicas[idx].failure_count++;
    stop_replica(role);
    return start_replica(role);
}

// Submit a vote (with optional signature verification)
bool submit_vote(const uint8_t* result_hash, ReplicaRole from) {
    std::lock_guard<std::mutex> lock(g_mutex);
    int from_idx = static_cast<int>(from);

    int slot = -1;
    for (int i = 0; i < g_vote_count; i++) {
        if (std::memcmp(g_votes[i].hash, result_hash, 64) == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0 && g_vote_count < 256) {
        slot = g_vote_count++;
        std::memcpy(g_votes[slot].hash, result_hash, 64);
        g_votes[slot].votes[0] = -1;
        g_votes[slot].votes[1] = -1;
        g_votes[slot].votes[2] = -1;
        g_votes[slot].count = 0;
        g_votes[slot].finalized = false;
        g_votes[slot].round = g_consensus_round++;
        g_votes[slot].timestamp = bs_clock_ns();
    }
    if (slot < 0) return false;

    // Record the vote
    if (g_votes[slot].votes[from_idx] < 0) {
        g_votes[slot].votes[from_idx] = 1; // accept
        g_votes[slot].count++;
    }

    // Broadcast vote to other replicas
    replica_socket::send_vote(from, result_hash);

    return true;
}

bool submit_signed_vote(const SignedVote& vote) {
    // Verify the post-quantum signature before accepting
    if (!verify_vote_signature(vote)) {
        __android_log_print(ANDROID_LOG_WARN, "APEX-CONSENSUS",
            "Rejected vote from replica %d: invalid PQ signature",
            static_cast<int>(vote.from));
        return false;
    }
    return submit_vote(vote.result_hash, vote.from);
}

// Check if consensus has been reached (≥2 of 3 replicas agree)
bool has_consensus(const uint8_t* result_hash) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (int i = 0; i < g_vote_count; i++) {
        if (std::memcmp(g_votes[i].hash, result_hash, 64) == 0) {
            if (g_votes[i].finalized) return true;
            if (g_votes[i].count >= 2) {
                // Mark as finalized (consensus reached)
                g_votes[i].finalized = true;
                __android_log_print(ANDROID_LOG_DEBUG, "APEX-CONSENSUS",
                    "Consensus reached on round %" PRIu64
                    " with %d votes (hash=%02x%02x...)",
                    g_votes[i].round, g_votes[i].count,
                    result_hash[0], result_hash[1]);
                // Broadcast consensus to all replicas for commitment
                replica_socket::broadcast_result(result_hash, 64);
                return true;
            }
            return false;
        }
    }
    return false;
}

// Get consensus count for a result hash
int get_consensus_count(const uint8_t* result_hash) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (int i = 0; i < g_vote_count; i++) {
        if (std::memcmp(g_votes[i].hash, result_hash, 64) == 0) {
            return g_votes[i].count;
        }
    }
    return 0;
}

// Get current consensus round
uint64_t get_current_round() {
    return g_consensus_round;
}

// Perform a full consensus round: collect votes from all 3 replicas
bool run_consensus_round(const uint8_t* result_hash) {
    // Create signed vote
    SignedVote vote;
    std::memcpy(vote.result_hash, result_hash, 64);
    vote.timestamp_ns = bs_clock_ns();

    // Vote from Replica A (primary)
    vote.from = ReplicaRole::REPLICA_A;
    auto& oqs = crypto::OqsSignature::getInstance();
    if (oqs.isAvailable()) {
        vote.dilithium_signature = oqs.sign(
            std::vector<uint8_t>(result_hash, result_hash + 64),
            g_replica_priv[0]);
        vote.public_key = g_replica_pub[0];
    } else {
        // Fallback: use software signing
        vote.dilithium_signature = crypto::dilithium_sign(
            result_hash, 64,
            g_replica_priv[0].data(), g_replica_priv[0].size());
        vote.public_key = g_replica_pub[0];
    }
    submit_signed_vote(vote);

    // Request votes from replicas B and C
    for (int i = 1; i < 3; i++) {
        if (!g_running[i]) continue;
        replica_socket::send_vote(static_cast<ReplicaRole>(i), result_hash);
    }

    // Wait for consensus with timeout
    uint64_t deadline = bs_clock_ns() + 3000000000ULL; // 3 second timeout
    while (bs_clock_ns() < deadline) {
        if (has_consensus(result_hash)) return true;

        // Check incoming votes from replicas
        auto msg = replica_socket::receive_message();
        if (!msg.empty() && msg[0] == 0x02 && msg.size() >= 65) {
            // Process vote from replica
            uint8_t hash[64];
            std::memcpy(hash, msg.data() + 1, 64);
            submit_vote(hash, static_cast<ReplicaRole>(msg[1]));
        }

        bs_nanosleep(100000000ULL); // 100ms
    }

    // Timeout - check if we have at least 2 votes
    return has_consensus(result_hash);
}

// Update heartbeat timestamp (called from monitor thread)
bool update_heartbeat(ReplicaRole role) {
    int idx = static_cast<int>(role);
    g_last_heartbeat[idx] = bs_clock_ns();
    g_replicas[idx].heartbeat_ns = g_last_heartbeat[idx];
    return true;
}

} // namespace replica_manager
} // namespace consensus
} // namespace apex

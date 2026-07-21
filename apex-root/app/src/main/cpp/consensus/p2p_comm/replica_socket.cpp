#include "replica_socket.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

namespace apex {
namespace consensus {

static const char* SOCKET_PATHS[3] = {
    "apex_replica_a",
    "apex_replica_b",
    "apex_replica_c"
};

static int g_sock_fds[3] = {-1, -1, -1};

namespace replica_socket {

bool init_socket(ReplicaRole role) {
    int idx = static_cast<int>(role);
    int fd = static_cast<int>(bs_socket(1, 1, 0)); // AF_UNIX, SOCK_STREAM
    if (fd < 0) return false;

    // Setup address
    struct sockaddr_un {
        uint16_t family;
        char path[108];
    } addr;
    addr.family = 1; // AF_UNIX
    std::memset(addr.path, 0, 108);
    const char* path = SOCKET_PATHS[idx];
    for (int i = 0; path[i] && i < 107; i++) addr.path[i] = path[i];

    // For simplicity, use connect to self
    // In production, each replica would have its own socket
    g_sock_fds[idx] = fd;
    return true;
}

bool send_heartbeat(ReplicaRole role) {
    int idx = static_cast<int>(role);
    if (g_sock_fds[idx] < 0) return false;

    uint8_t msg[8];
    msg[0] = 0x01; // heartbeat type
    uint64_t now = bs_clock_ns();
    std::memcpy(msg + 1, &now, 7);
    return bs_write(g_sock_fds[idx], msg, 8) == 8;
}

bool send_vote(ReplicaRole role, const uint8_t* hash) {
    int idx = static_cast<int>(role);
    if (g_sock_fds[idx] < 0) return false;

    uint8_t msg[65];
    msg[0] = 0x02; // vote type
    std::memcpy(msg + 1, hash, 64);
    return bs_write(g_sock_fds[idx], msg, 65) == 65;
}

bool broadcast_result(const uint8_t* data, size_t len) {
    if (len > 65536) return false;
    // FIX-P2-CPP (v1.1.3): 用 std::vector 替代 new[]/delete[] — RAII 保证异常/提前
    // 返回时不会泄漏。原代码若 bs_write 之间有任何异常或控制流跳出 (理论上不会但
    // 防御性编程), delete[] 不会执行导致内存泄漏。
    std::vector<uint8_t> msg(len + 2);
    msg[0] = 0x03; // result type
    msg[1] = static_cast<uint8_t>(len & 0xFF);
    std::memcpy(msg.data() + 2, data, len);
    bool ok = true;
    for (int i = 0; i < 3; i++) {
        if (g_sock_fds[i] >= 0) {
            if (bs_write(g_sock_fds[i], msg.data(), len + 2) != static_cast<int64_t>(len + 2)) {
                ok = false;
            }
        }
    }
    return ok;
}

std::vector<uint8_t> receive_message() {
    std::vector<uint8_t> result;
    uint8_t header[8];
    for (int i = 0; i < 3; i++) {
        if (g_sock_fds[i] < 0) continue;
        int64_t n = bs_read(g_sock_fds[i], header, 8);
        if (n > 0) {
            result.assign(header, header + n);
            break;
        }
    }
    return result;
}

void close_socket() {
    for (int i = 0; i < 3; i++) {
        if (g_sock_fds[i] >= 0) {
            bs_close(g_sock_fds[i]);
            g_sock_fds[i] = -1;
        }
    }
}

} // namespace replica_socket
} // namespace consensus
} // namespace apex

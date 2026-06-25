#include "../ipc.h"
#include "../../crypto/core/crypto_core.h"
#include "../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>

/* 安全随机数填充（复用 ipc_crypto.c 中的声明）*/
static void secure_random_fill(uint8_t *buf, size_t len) {
    long ret = bare_syscall(278, buf, len, 0);
    if (ret < 0) {
        for (size_t i = 0; i < len; i += 8) {
            uint64_t val = bare_rdtsc() ^ ((uint64_t)bare_getpid() << 32);
            size_t copy = (len - i) < 8 ? (len - i) : 8;
            bare_memcpy(buf + i, &val, copy);
        }
    }
}

int ipc_send_message(int sock_fd, ipc_session_t *session,
                     ipc_msg_type_t type, const uint8_t *payload,
                     uint32_t payload_len) {
    ipc_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.type = type;
    msg.header.seq = __sync_fetch_and_add(&session->next_seq, 1);
    msg.header.payload_len = payload_len;
    uint8_t mac_input[8 + IPC_MAX_MSG_SIZE];
    memcpy(mac_input, &msg.header.type, 8);
    if (payload && payload_len > 0)
        memcpy(mac_input + 8, payload, payload_len < IPC_MAX_MSG_SIZE ? payload_len : IPC_MAX_MSG_SIZE);
    ipc_compute_mac(mac_input, 8 + payload_len, session->session_key, msg.header.mac);
    if (payload && payload_len > 0)
        memcpy(msg.payload, payload, payload_len);
    size_t total = sizeof(ipc_header_t) + payload_len;
    if (write(sock_fd, &msg, total) != (ssize_t)total)
        return -1;
    return 0;
}

int ipc_recv_message(int sock_fd, ipc_session_t *session,
                     ipc_message_t *msg) {
    memset(msg, 0, sizeof(ipc_message_t));
    if (read(sock_fd, &msg->header, sizeof(ipc_header_t)) != sizeof(ipc_header_t))
        return -1;
    if (msg->header.payload_len > IPC_MAX_MSG_SIZE)
        return -1;
    if (msg->header.payload_len > 0) {
        if (read(sock_fd, msg->payload, msg->header.payload_len) != (ssize_t)msg->header.payload_len)
            return -1;
    }
    uint8_t mac_input[8 + IPC_MAX_MSG_SIZE];
    memcpy(mac_input, &msg->header.type, 8);
    if (msg->header.payload_len > 0)
        memcpy(mac_input + 8, msg->payload, msg->header.payload_len);
    uint8_t computed_mac[16];
    ipc_compute_mac(mac_input, 8 + msg->header.payload_len, session->session_key, computed_mac);
    if (memcmp(computed_mac, msg->header.mac, 16) != 0)
        return -2;
    return 0;
}

int ipc_handshake_server(int client_fd, ipc_session_t *session) {
    uint8_t server_nonce[16], client_nonce[16];
    secure_random_fill(server_nonce, 16);
    if (write(client_fd, server_nonce, 16) != 16) return -1;
    if (read(client_fd, client_nonce, 16) != 16) return -1;
    uint8_t shared_secret[32];
    for (int i = 0; i < 16; i++) shared_secret[i] = server_nonce[i] ^ client_nonce[i];
    for (int i = 0; i < 16; i++) shared_secret[16 + i] = client_nonce[i] ^ server_nonce[15 - i];
    sha3_256(shared_secret, 32, session->session_key);
    session->sock_fd = client_fd;
    session->next_seq = 0;
    return 0;
}

int ipc_handshake_client(int sock_fd, ipc_session_t *session) {
    uint8_t server_nonce[16], client_nonce[16];
    if (read(sock_fd, server_nonce, 16) != 16) return -1;
    secure_random_fill(client_nonce, 16);
    if (write(sock_fd, client_nonce, 16) != 16) return -1;
    uint8_t shared_secret[32];
    for (int i = 0; i < 16; i++) shared_secret[i] = server_nonce[i] ^ client_nonce[i];
    for (int i = 0; i < 16; i++) shared_secret[16 + i] = client_nonce[i] ^ server_nonce[15 - i];
    sha3_256(shared_secret, 32, session->session_key);
    session->sock_fd = sock_fd;
    session->next_seq = 0;
    return 0;
}
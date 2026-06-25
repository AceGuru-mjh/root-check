#ifndef IPC_COMM_H
#define IPC_COMM_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_MAX_MSG_SIZE 65536
#define IPC_DEFAULT_SOCKET_PATH "/data/local/tmp/.rootguard_ipc"
#define IPC_CONNECT_TIMEOUT_MS 5000
#define IPC_RECV_TIMEOUT_MS 10000

typedef enum {
    IPC_MSG_HEARTBEAT = 0,
    IPC_MSG_DETECT_REQUEST = 1,
    IPC_MSG_DETECT_RESULT = 2,
    IPC_MSG_PLUGIN_LOAD = 3,
    IPC_MSG_PLUGIN_UNLOAD = 4,
    IPC_MSG_PLUGIN_RESULT = 5,
    IPC_MSG_DAEMON_STATUS = 6,
    IPC_MSG_SHUTDOWN = 7
} ipc_msg_type_t;

typedef struct {
    ipc_msg_type_t type;
    uint32_t seq;
    uint32_t payload_len;
    uint8_t mac[16];
} __attribute__((packed)) ipc_header_t;

typedef struct {
    ipc_header_t header;
    uint8_t payload[IPC_MAX_MSG_SIZE];
} ipc_message_t;

typedef struct {
    int sock_fd;
    char *socket_path;
    uint8_t session_key[32];
    uint32_t next_seq;
} ipc_session_t;

int ipc_server_start(const char *socket_path);
int ipc_server_accept(int server_fd);
void ipc_server_stop(int server_fd);

int ipc_client_connect(const char *socket_path);
void ipc_client_disconnect(int sock_fd);

int ipc_send_message(int sock_fd, ipc_session_t *session,
                     ipc_msg_type_t type, const uint8_t *payload,
                     uint32_t payload_len);
int ipc_recv_message(int sock_fd, ipc_session_t *session,
                     ipc_message_t *msg);

int ipc_handshake_server(int client_fd, ipc_session_t *session);
int ipc_handshake_client(int sock_fd, ipc_session_t *session);

/* IPC crypto functions (implemented in ipc_crypto.c) */
int  ipc_encrypt_payload(const uint8_t *plain, uint32_t plain_len,
                         const uint8_t *key, uint8_t *cipher,
                         uint8_t *tag, uint8_t *iv);
int  ipc_decrypt_payload(const uint8_t *cipher, uint32_t cipher_len,
                         const uint8_t *key, const uint8_t *iv,
                         const uint8_t *tag, uint8_t *plain);
void ipc_compute_mac(const uint8_t *data, uint32_t data_len,
                     const uint8_t *key, uint8_t mac[16]);
int  ipc_verify_mac(const uint8_t *data, uint32_t data_len,
                    const uint8_t *key, const uint8_t mac[16]);

#ifdef __cplusplus
}
#endif

#endif /* IPC_COMM_H */
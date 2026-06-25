/*
 * unix_socket_client.h - Unix Domain Socket 客户端接口
 *
 * 用于连接 IPC 服务器（Sandbox 或 Daemon）。
 * 所有通信均使用 Unix Socket，不使用 Binder。
 */

#ifndef UNIX_SOCKET_CLIENT_H
#define UNIX_SOCKET_CLIENT_H

#include "../core/ipc_protocol.h"
#include "../core/ipc_crypto.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 客户端上下文 ===================== */

/*
 * Unix Socket 客户端上下文
 * 每个客户端连接维护一个上下文，包含 socket 连接、加密上下文等。
 */
typedef struct {
    int                 sock_fd;            /* Socket 文件描述符 */
    char                socket_path[256];   /* Socket 文件路径 */
    ipc_crypto_ctx_t    crypto_ctx;         /* 加密上下文 */
    uint64_t            next_seq;           /* 下一条消息的序列号 */
    int                 is_connected;       /* 连接状态 */
} unix_socket_client_t;

/* ===================== 客户端 API ===================== */

/*
 * connect_to_server:
 *   连接到 Unix Domain Socket 服务器。
 *
 *   流程：
 *   1. 创建 AF_UNIX socket
 *   2. 连接到指定路径
 *   3. 初始化加密上下文
 *   4. 重置序列号为 1
 *
 * 参数：
 *   client       - 客户端上下文
 *   socket_path  - Socket 文件路径（如 /data/data/com.rootguard/socket/sandbox_ipc）
 *   session_key  - 32 字节 AES-256 会话密钥（与服务器共享）
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_SOCKET_CREATE  - 创建 socket 失败
 *   IPC_ERR_SOCKET_CONNECT - 连接失败
 */
__attribute__((visibility("default")))
ipc_status_t connect_to_server(unix_socket_client_t *client,
                                const char *socket_path,
                                const uint8_t *session_key);

/*
 * client_send_message:
 *   发送加密的 IPC 消息。
 *
 *   流程：
 *   1. 设置消息头（magic、type、seq、timestamp）
 *   2. 调用 encrypt_message() 加密载荷
 *   3. 调用 serialize_message() 序列化
 *   4. 通过 socket 发送
 *   5. 递增序列号
 *
 * 参数：
 *   client       - 客户端上下文
 *   msg_type     - 消息类型
 *   payload      - 明文载荷
 *   payload_len  - 载荷长度
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_SOCKET_SEND    - 发送失败
 *   IPC_ERR_SOCKET_CLOSED  - 未连接
 *   IPC_ERR_CRYPTO_ENCRYPT - 加密失败
 */
__attribute__((visibility("default")))
ipc_status_t client_send_message(unix_socket_client_t *client,
                                  ipc_msg_type_t msg_type,
                                  const uint8_t *payload,
                                  size_t payload_len);

/*
 * client_recv_message:
 *   接收并解密 IPC 消息。
 *
 *   流程：
 *   1. 从 socket 接收数据
 *   2. 调用 deserialize_message() 反序列化
 *   3. 调用 decrypt_message() 解密并校验（序列号、时间戳、tag）
 *   4. 若校验失败，丢弃数据包并触发安全告警
 *
 * 参数：
 *   client       - 客户端上下文
 *   msg          - 输出消息容器（调用方需分配 payload 缓冲区）
 *   timeout_ms   - 超时时间（毫秒，0 = 永久阻塞）
 *
 * 返回：
 *   IPC_OK                     - 成功
 *   IPC_ERR_INVALID_ARG        - 参数错误
 *   IPC_ERR_SOCKET_RECV        - 接收失败
 *   IPC_ERR_SOCKET_CLOSED      - 连接已关闭
 *   IPC_ERR_DESERIALIZE        - 反序列化失败
 *   IPC_ERR_REPLAY_SEQ_OLD     - 重放攻击（序列号过旧）
 *   IPC_ERR_REPLAY_SEQ_DUP     - 重放攻击（序列号重复）
 *   IPC_ERR_TIMESTAMP_EXPIRED  - 时间戳过期
 *   IPC_ERR_TAG_MISMATCH       - 认证标签不匹配（篡改）
 */
__attribute__((visibility("default")))
ipc_status_t client_recv_message(unix_socket_client_t *client,
                                  ipc_message_t *msg,
                                  uint32_t timeout_ms);

/*
 * close_connection:
 *   关闭客户端连接，释放资源。
 *
 *   流程：
 *   1. 关闭 socket 连接
 *   2. 清理加密上下文
 */
__attribute__((visibility("default")))
void close_connection(unix_socket_client_t *client);

/*
 * get_client_fd:
 *   获取客户端 socket 文件描述符（用于 select/poll）。
 */
__attribute__((visibility("default")))
int get_client_fd(const unix_socket_client_t *client);

/*
 * is_connected:
 *   检查客户端是否已连接。
 */
__attribute__((visibility("default")))
int is_connected(const unix_socket_client_t *client);

#ifdef __cplusplus
}
#endif

#endif /* UNIX_SOCKET_CLIENT_H */

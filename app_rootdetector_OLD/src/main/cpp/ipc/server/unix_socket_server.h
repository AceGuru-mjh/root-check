/*
 * unix_socket_server.h - Unix Domain Socket 服务端接口
 *
 * 支持两种模式：
 *   - Sandbox 服务器：接收 UI 进程的任务派发
 *   - Daemon 服务器：接收 Sandbox 的检测结果和签名请求
 *
 * 所有通信均使用 Unix Socket，不使用 Binder。
 */

#ifndef UNIX_SOCKET_SERVER_H
#define UNIX_SOCKET_SERVER_H

#include "../core/ipc_protocol.h"
#include "../core/ipc_crypto.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 服务器上下文 ===================== */

/*
 * Unix Socket 服务器上下文
 * 每个服务器实例维护一个上下文，包含监听 socket、客户端连接、加密上下文等。
 */
typedef struct {
    int                 listen_fd;          /* 监听 socket 文件描述符 */
    int                 client_fd;          /* 已接受的客户端连接 */
    char                socket_path[256];   /* Socket 文件路径 */
    ipc_crypto_ctx_t    crypto_ctx;         /* 加密上下文（每个连接一个） */
    uint64_t            next_seq;           /* 下一条消息的序列号 */
    int                 is_running;         /* 服务器运行状态 */
} unix_socket_server_t;

/* ===================== 服务器 API ===================== */

/*
 * create_server_socket:
 *   创建 Unix Domain Socket 服务器。
 *
 *   流程：
 *   1. 删除已存在的 socket 文件
 *   2. 创建 AF_UNIX socket
 *   3. 绑定到指定路径
 *   4. 设置权限为 0700（仅所有者可访问）
 *   5. 开始监听
 *
 * 参数：
 *   server       - 服务器上下文
 *   socket_path  - Socket 文件路径（如 /data/data/com.rootguard/socket/sandbox_ipc）
 *   session_key  - 32 字节 AES-256 会话密钥（与客户端共享）
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_SOCKET_CREATE  - 创建 socket 失败
 *   IPC_ERR_SOCKET_BIND    - 绑定失败
 *   IPC_ERR_SOCKET_LISTEN  - 监听失败
 */
__attribute__((visibility("default")))
ipc_status_t create_server_socket(unix_socket_server_t *server,
                                   const char *socket_path,
                                   const uint8_t *session_key);

/*
 * accept_connection:
 *   接受客户端连接。
 *
 *   流程：
 *   1. 调用 accept() 阻塞等待连接
 *   2. 初始化客户端的加密上下文
 *   3. 重置序列号为 0
 *
 * 参数：
 *   server       - 服务器上下文
 *   timeout_ms   - 超时时间（毫秒，0 = 永久阻塞）
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_SOCKET_ACCEPT  - 接受连接失败
 *   IPC_ERR_SOCKET_CLOSED  - 服务器未启动
 */
__attribute__((visibility("default")))
ipc_status_t accept_connection(unix_socket_server_t *server,
                                uint32_t timeout_ms);

/*
 * send_message:
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
 *   server       - 服务器上下文
 *   msg_type     - 消息类型
 *   payload      - 明文载荷
 *   payload_len  - 载荷长度
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_SOCKET_SEND    - 发送失败
 *   IPC_ERR_CRYPTO_ENCRYPT - 加密失败
 */
__attribute__((visibility("default")))
ipc_status_t server_send_message(unix_socket_server_t *server,
                                  ipc_msg_type_t msg_type,
                                  const uint8_t *payload,
                                  size_t payload_len);

/*
 * recv_message:
 *   接收并解密 IPC 消息。
 *
 *   流程：
 *   1. 从 socket 接收数据
 *   2. 调用 deserialize_message() 反序列化
 *   3. 调用 decrypt_message() 解密并校验（序列号、时间戳、tag）
 *   4. 若校验失败，丢弃数据包并触发安全告警
 *
 * 参数：
 *   server       - 服务器上下文
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
ipc_status_t server_recv_message(unix_socket_server_t *server,
                                  ipc_message_t *msg,
                                  uint32_t timeout_ms);

/*
 * close_server:
 *   关闭服务器，释放资源。
 *
 *   流程：
 *   1. 关闭客户端连接
 *   2. 关闭监听 socket
 *   3. 删除 socket 文件
 *   4. 清理加密上下文
 */
__attribute__((visibility("default")))
void close_server(unix_socket_server_t *server);

/*
 * get_server_fd:
 *   获取服务器监听 socket 文件描述符（用于 select/poll）。
 */
__attribute__((visibility("default")))
int get_server_fd(const unix_socket_server_t *server);

/*
 * get_client_fd:
 *   获取客户端连接文件描述符（用于 select/poll）。
 */
__attribute__((visibility("default")))
int get_client_fd(const unix_socket_server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* UNIX_SOCKET_SERVER_H */

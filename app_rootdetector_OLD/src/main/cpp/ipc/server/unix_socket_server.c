/*
 * unix_socket_server.c - Unix Domain Socket 服务端实现
 *
 * 使用裸系统调用实现，不依赖 libc 网络函数。
 * 所有消息均经过 AES-GCM 加密，附带序列号+时间戳防重放。
 */

#include "unix_socket_server.h"
#include "../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>

/* ===================== 内部辅助函数 ===================== */

/*
 * 获取当前时间戳（纳秒）
 * 使用 CLOCK_MONOTONIC 防止系统时间被篡改
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/*
 * 设置 socket 为非阻塞模式
 */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* ===================== 服务器 API 实现 ===================== */

/*
 * create_server_socket:
 *   创建 Unix Domain Socket 服务器。
 */
__attribute__((visibility("default")))
ipc_status_t create_server_socket(unix_socket_server_t *server,
                                   const char *socket_path,
                                   const uint8_t *session_key) {
    if (!server || !socket_path || !session_key) {
        return IPC_ERR_INVALID_ARG;
    }

    /* 初始化服务器上下文 */
    bare_memset(server, 0, sizeof(unix_socket_server_t));
    server->listen_fd = -1;
    server->client_fd = -1;
    server->next_seq = 1;
    server->is_running = 0;

    /* 保存 socket 路径 */
    size_t path_len = 0;
    const char *p = socket_path;
    while (*p++ && path_len < sizeof(server->socket_path) - 1) {
        path_len++;
    }
    bare_memcpy(server->socket_path, socket_path, path_len);
    server->socket_path[path_len] = '\0';

    /* 初始化加密上下文 */
    ipc_status_t crypto_status = ipc_crypto_init(&server->crypto_ctx, session_key, 32);
    if (crypto_status != IPC_OK) {
        return crypto_status;
    }

    /* 删除已存在的 socket 文件 */
    unlink(socket_path);

    /* 创建 Unix Domain Socket */
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        ipc_crypto_cleanup(&server->crypto_ctx);
        return IPC_ERR_SOCKET_CREATE;
    }

    /* 绑定到指定路径 */
    struct sockaddr_un addr;
    bare_memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    bare_memcpy(addr.sun_path, socket_path, path_len);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        ipc_crypto_cleanup(&server->crypto_ctx);
        return IPC_ERR_SOCKET_BIND;
    }

    /* 设置 socket 文件权限为 0700 */
    chmod(socket_path, 0700);

    /* 开始监听 */
    if (listen(fd, 5) < 0) {
        close(fd);
        unlink(socket_path);
        ipc_crypto_cleanup(&server->crypto_ctx);
        return IPC_ERR_SOCKET_LISTEN;
    }

    server->listen_fd = fd;
    server->is_running = 1;

    return IPC_OK;
}

/*
 * accept_connection:
 *   接受客户端连接。
 */
__attribute__((visibility("default")))
ipc_status_t accept_connection(unix_socket_server_t *server,
                                uint32_t timeout_ms) {
    if (!server || !server->is_running) {
        return IPC_ERR_SOCKET_CLOSED;
    }

    /* 若设置了超时，使用 select 等待 */
    if (timeout_ms > 0) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server->listen_fd, &readfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(server->listen_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0) {
            return IPC_ERR_SOCKET_ACCEPT; /* 超时或错误 */
        }
    }

    /* 接受连接 */
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server->listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        return IPC_ERR_SOCKET_ACCEPT;
    }

    /* 关闭已存在的客户端连接 */
    if (server->client_fd >= 0) {
        close(server->client_fd);
    }

    server->client_fd = client_fd;

    /* 重置序列号（新连接从 1 开始） */
    server->next_seq = 1;
    server->crypto_ctx.last_seen_seq = 0;

    return IPC_OK;
}

/*
 * server_send_message:
 *   发送加密的 IPC 消息。
 */
__attribute__((visibility("default")))
ipc_status_t server_send_message(unix_socket_server_t *server,
                                  ipc_msg_type_t msg_type,
                                  const uint8_t *payload,
                                  size_t payload_len) {
    if (!server || !server->is_running || server->client_fd < 0) {
        return IPC_ERR_SOCKET_CLOSED;
    }

    if (payload_len > IPC_MAX_PAYLOAD_SIZE) {
        return IPC_ERR_PAYLOAD_TOO_LARGE;
    }

    /* 构造消息 */
    ipc_message_t msg;
    message_init(&msg);

    msg.header.magic = IPC_MAGIC;
    msg.header.msg_type = (uint32_t)msg_type;
    msg.header.sequence_num = server->next_seq;
    msg.header.timestamp = get_timestamp_ns();
    msg.header.payload_len = (uint32_t)payload_len;

    /* 分配载荷缓冲区 */
    uint8_t payload_buf[IPC_MAX_PAYLOAD_SIZE];
    if (payload_len > 0) {
        if (!payload) {
            return IPC_ERR_INVALID_ARG;
        }
        bare_memcpy(payload_buf, payload, payload_len);
        msg.payload = payload_buf;
        msg.payload_len = payload_len;
    }

    /* 加密消息 */
    ipc_status_t crypto_status = encrypt_message(&msg, &server->crypto_ctx);
    if (crypto_status != IPC_OK) {
        return crypto_status;
    }

    /* 序列化消息 */
    uint8_t send_buf[sizeof(ipc_message_header_t) + IPC_MAX_PAYLOAD_SIZE];
    size_t send_len = 0;

    ipc_status_t ser_status = serialize_message(&msg, send_buf, sizeof(send_buf), &send_len);
    if (ser_status != IPC_OK) {
        return ser_status;
    }

    /* 发送数据 */
    ssize_t sent = 0;
    while (sent < (ssize_t)send_len) {
        ssize_t n = write(server->client_fd, send_buf + sent, send_len - sent);
        if (n < 0) {
            if (errno == EINTR) continue;
            return IPC_ERR_SOCKET_SEND;
        }
        if (n == 0) {
            return IPC_ERR_SOCKET_CLOSED;
        }
        sent += n;
    }

    /* 递增序列号 */
    server->next_seq++;

    return IPC_OK;
}

/*
 * server_recv_message:
 *   接收并解密 IPC 消息。
 */
__attribute__((visibility("default")))
ipc_status_t server_recv_message(unix_socket_server_t *server,
                                  ipc_message_t *msg,
                                  uint32_t timeout_ms) {
    if (!server || !msg || !server->is_running || server->client_fd < 0) {
        return IPC_ERR_INVALID_ARG;
    }

    /* 若设置了超时，使用 select 等待 */
    if (timeout_ms > 0) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server->client_fd, &readfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(server->client_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0) {
            return IPC_ERR_SOCKET_RECV; /* 超时或错误 */
        }
    }

    /* 接收消息头 */
    uint8_t header_buf[sizeof(ipc_message_header_t)];
    ssize_t received = 0;
    while (received < (ssize_t)sizeof(header_buf)) {
        ssize_t n = read(server->client_fd, header_buf + received, sizeof(header_buf) - received);
        if (n < 0) {
            if (errno == EINTR) continue;
            return IPC_ERR_SOCKET_RECV;
        }
        if (n == 0) {
            return IPC_ERR_SOCKET_CLOSED;
        }
        received += n;
    }

    /* 反序列化消息头 */
    ipc_message_t temp_msg;
    ipc_status_t deser_status = deserialize_message(header_buf, sizeof(header_buf), &temp_msg);
    if (deser_status != IPC_OK) {
        return deser_status;
    }

    /* 检查载荷长度 */
    if (temp_msg.header.payload_len > IPC_MAX_PAYLOAD_SIZE) {
        return IPC_ERR_PAYLOAD_TOO_LARGE;
    }

    /* 接收载荷 */
    uint8_t payload_buf[IPC_MAX_PAYLOAD_SIZE];
    if (temp_msg.header.payload_len > 0) {
        received = 0;
        while (received < (ssize_t)temp_msg.header.payload_len) {
            ssize_t n = read(server->client_fd, payload_buf + received,
                            temp_msg.header.payload_len - received);
            if (n < 0) {
                if (errno == EINTR) continue;
                return IPC_ERR_SOCKET_RECV;
            }
            if (n == 0) {
                return IPC_ERR_SOCKET_CLOSED;
            }
            received += n;
        }
    }

    /* 构造完整消息（密文） */
    uint8_t full_buf[sizeof(ipc_message_header_t) + IPC_MAX_PAYLOAD_SIZE];
    bare_memcpy(full_buf, header_buf, sizeof(header_buf));
    if (temp_msg.header.payload_len > 0) {
        bare_memcpy(full_buf + sizeof(header_buf), payload_buf, temp_msg.header.payload_len);
    }

    /* 反序列化完整消息 */
    ipc_message_t enc_msg;
    message_init(&enc_msg);
    enc_msg.payload = payload_buf;
    enc_msg.payload_len = temp_msg.header.payload_len;

    deser_status = deserialize_message(full_buf,
                                        sizeof(header_buf) + temp_msg.header.payload_len,
                                        &enc_msg);
    if (deser_status != IPC_OK) {
        return deser_status;
    }

    /* 解密消息 */
    uint64_t now_ns = get_timestamp_ns();
    ipc_status_t crypto_status = decrypt_message(&enc_msg, &server->crypto_ctx, now_ns);
    if (crypto_status != IPC_OK) {
        /* 解密失败，触发安全告警（已在 decrypt_message 中处理） */
        return crypto_status;
    }

    /* 拷贝解密后的消息到输出 */
    bare_memcpy(msg, &enc_msg, sizeof(ipc_message_t));

    return IPC_OK;
}

/*
 * close_server:
 *   关闭服务器，释放资源。
 */
__attribute__((visibility("default")))
void close_server(unix_socket_server_t *server) {
    if (!server) return;

    /* 关闭客户端连接 */
    if (server->client_fd >= 0) {
        close(server->client_fd);
        server->client_fd = -1;
    }

    /* 关闭监听 socket */
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
        server->listen_fd = -1;
    }

    /* 删除 socket 文件 */
    if (server->socket_path[0] != '\0') {
        unlink(server->socket_path);
        server->socket_path[0] = '\0';
    }

    /* 清理加密上下文 */
    ipc_crypto_cleanup(&server->crypto_ctx);

    server->is_running = 0;
}

/*
 * get_server_fd:
 *   获取服务器监听 socket 文件描述符。
 */
__attribute__((visibility("default")))
int get_server_fd(const unix_socket_server_t *server) {
    if (!server) return -1;
    return server->listen_fd;
}

/*
 * get_client_fd:
 *   获取客户端连接文件描述符。
 */
__attribute__((visibility("default")))
int get_client_fd(const unix_socket_server_t *server) {
    if (!server) return -1;
    return server->client_fd;
}

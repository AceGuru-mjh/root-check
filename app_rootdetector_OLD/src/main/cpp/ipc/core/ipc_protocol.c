/*
 * ipc_protocol.c - IPC 协议序列化/反序列化实现
 *
 * 负责消息的打包、解包、防重放校验、时间戳校验。
 * 纯 C11 实现，不依赖 libc 网络函数。
 */

#include "ipc_protocol.h"
#include "../cpp/bare_syscall/bare_syscall.h"
#include <string.h>

/* ===================== 内部辅助函数 ===================== */

/*
 * 构造 AAD（附加认证数据）
 * AAD 用于 AES-GCM 加密，绑定消息类型、序列号、时间戳，防止篡改。
 * 布局：msg_type(4) || seq_low(4) || seq_high(4) || ts_low(4) = 16 字节
 */
void build_aad(const ipc_message_header_t *hdr, uint8_t out_aad[IPC_AAD_LEN]) {
    if (!hdr || !out_aad) return;

    /* msg_type (4 bytes, little-endian) */
    out_aad[0] = (uint8_t)(hdr->msg_type & 0xFF);
    out_aad[1] = (uint8_t)((hdr->msg_type >> 8) & 0xFF);
    out_aad[2] = (uint8_t)((hdr->msg_type >> 16) & 0xFF);
    out_aad[3] = (uint8_t)((hdr->msg_type >> 24) & 0xFF);

    /* sequence_num low 4 bytes */
    out_aad[4] = (uint8_t)(hdr->sequence_num & 0xFF);
    out_aad[5] = (uint8_t)((hdr->sequence_num >> 8) & 0xFF);
    out_aad[6] = (uint8_t)((hdr->sequence_num >> 16) & 0xFF);
    out_aad[7] = (uint8_t)((hdr->sequence_num >> 24) & 0xFF);

    /* sequence_num high 4 bytes */
    out_aad[8]  = (uint8_t)((hdr->sequence_num >> 32) & 0xFF);
    out_aad[9]  = (uint8_t)((hdr->sequence_num >> 40) & 0xFF);
    out_aad[10] = (uint8_t)((hdr->sequence_num >> 48) & 0xFF);
    out_aad[11] = (uint8_t)((hdr->sequence_num >> 56) & 0xFF);

    /* timestamp low 4 bytes */
    out_aad[12] = (uint8_t)(hdr->timestamp & 0xFF);
    out_aad[13] = (uint8_t)((hdr->timestamp >> 8) & 0xFF);
    out_aad[14] = (uint8_t)((hdr->timestamp >> 16) & 0xFF);
    out_aad[15] = (uint8_t)((hdr->timestamp >> 24) & 0xFF);
}

/*
 * 初始化消息容器
 */
void message_init(ipc_message_t *msg) {
    if (!msg) return;
    bare_memset(msg, 0, sizeof(ipc_message_t));
    msg->payload = NULL;
    msg->payload_len = 0;
    msg->is_encrypted = 0;
}

/*
 * 清理消息容器
 */
void message_free(ipc_message_t *msg) {
    if (!msg) return;
    if (msg->payload) {
        /* 安全清零后释放（假设 payload 由 malloc 分配） */
        bare_memset(msg->payload, 0, msg->payload_len);
        /* 注意：这里不能直接 free，因为 bare_syscall 层没有提供 free。
         * 实际使用时，调用方需确保 payload 的分配/释放配对。
         * 此处仅清零敏感数据。 */
        msg->payload = NULL;
    }
    msg->payload_len = 0;
    msg->is_encrypted = 0;
}

/* ===================== 序列化 ===================== */

/*
 * serialize_message:
 *   将 ipc_message_t 序列化为字节流。
 *   输出格式：[ipc_message_header_t][ciphertext]
 *
 * 前提条件：
 *   - msg->header.magic 已设置为 IPC_MAGIC
 *   - msg->header.msg_type 已设置
 *   - msg->header.sequence_num 已设置
 *   - msg->header.timestamp 已设置
 *   - msg->payload 已加密（is_encrypted=1）或为 NULL
 *   - msg->header.tag 已填充（若 is_encrypted=1）
 *   - msg->header.aad 已填充
 */
ipc_status_t serialize_message(const ipc_message_t *msg,
                               uint8_t *out_buf,
                               size_t out_size,
                               size_t *out_len) {
    if (!msg || !out_buf || !out_len) {
        return IPC_ERR_INVALID_ARG;
    }

    /* 校验 magic */
    if (msg->header.magic != IPC_MAGIC) {
        return IPC_ERR_BAD_MAGIC;
    }

    /* 校验消息类型 */
    if (msg->header.msg_type == IPC_MSG_NONE ||
        msg->header.msg_type >= IPC_MSG_TYPE_MAX) {
        return IPC_ERR_INVALID_ARG;
    }

    /* 计算总长度 */
    size_t header_len = sizeof(ipc_message_header_t);
    size_t total_len = header_len + msg->header.payload_len;

    /* 检查缓冲区是否足够 */
    if (out_size < total_len) {
        return IPC_ERR_BUFFER_TOO_SMALL;
    }

    /* 检查载荷长度是否超限 */
    if (msg->header.payload_len > IPC_MAX_PAYLOAD_SIZE) {
        return IPC_ERR_PAYLOAD_TOO_LARGE;
    }

    /* 拷贝消息头 */
    bare_memcpy(out_buf, &msg->header, header_len);

    /* 拷贝密文载荷（如果有） */
    if (msg->header.payload_len > 0) {
        if (!msg->payload) {
            return IPC_ERR_INVALID_ARG;
        }
        bare_memcpy(out_buf + header_len, msg->payload, msg->header.payload_len);
    }

    *out_len = total_len;
    return IPC_OK;
}

/* ===================== 反序列化 ===================== */

/*
 * deserialize_message:
 *   从字节流中解析消息。
 *   返回的 msg->is_encrypted = 1，调用方需自行解密。
 *
 * 注意：
 *   - 本函数会校验 magic
 *   - 不会解密、不会校验序列号/时间戳
 *   - msg->payload 需要调用方分配内存（建议 IPC_MAX_PAYLOAD_SIZE）
 */
ipc_status_t deserialize_message(const uint8_t *in_buf,
                                 size_t in_len,
                                 ipc_message_t *msg) {
    if (!in_buf || !msg) {
        return IPC_ERR_INVALID_ARG;
    }

    size_t header_len = sizeof(ipc_message_header_t);

    /* 检查最小长度 */
    if (in_len < header_len) {
        return IPC_ERR_DESERIALIZE;
    }

    /* 初始化消息容器 */
    message_init(msg);

    /* 拷贝消息头 */
    bare_memcpy(&msg->header, in_buf, header_len);

    /* 校验 magic */
    if (msg->header.magic != IPC_MAGIC) {
        return IPC_ERR_BAD_MAGIC;
    }

    /* 校验消息类型 */
    if (msg->header.msg_type == IPC_MSG_NONE ||
        msg->header.msg_type >= IPC_MSG_TYPE_MAX) {
        return IPC_ERR_DESERIALIZE;
    }

    /* 校验载荷长度 */
    if (msg->header.payload_len > IPC_MAX_PAYLOAD_SIZE) {
        return IPC_ERR_PAYLOAD_TOO_LARGE;
    }

    /* 检查实际长度是否匹配 */
    size_t expected_len = header_len + msg->header.payload_len;
    if (in_len < expected_len) {
        return IPC_ERR_DESERIALIZE;
    }

    /* 标记为加密状态（调用方需解密） */
    msg->is_encrypted = 1;
    msg->payload_len = msg->header.payload_len;

    /*
     * 注意：此处不分配 payload 内存，也不拷贝密文。
     * 调用方应在栈上或堆上准备缓冲区，并将指针赋给 msg->payload，
     * 然后由 decrypt_message() 填充明文。
     *
     * 若需在此处拷贝密文，可取消下方注释：
     *
     * if (msg->payload_len > 0) {
     *     msg->payload = (uint8_t *)malloc(msg->payload_len);
     *     if (!msg->payload) return IPC_ERR_INTERNAL;
     *     bare_memcpy(msg->payload, in_buf + header_len, msg->payload_len);
     * }
     */

    return IPC_OK;
}

/* ===================== 防重放校验 ===================== */

/*
 * validate_sequence:
 *   校验序列号是否单调递增（防重放）。
 *   last_seen_seq 由调用方维护，初始值为 0。
 *
 * 规则：
 *   - seq 必须 > last_seen_seq
 *   - 校验通过后，更新 *last_seen_seq = seq
 */
ipc_status_t validate_sequence(uint64_t seq, uint64_t *last_seen_seq) {
    if (!last_seen_seq) {
        return IPC_ERR_INVALID_ARG;
    }

    /* 序列号必须严格递增 */
    if (seq <= *last_seen_seq) {
        /* 判断是重复还是回退 */
        if (seq == *last_seen_seq) {
            return IPC_ERR_REPLAY_SEQ_DUP;
        } else {
            return IPC_ERR_REPLAY_SEQ_OLD;
        }
    }

    /* 更新最后见过的序列号 */
    *last_seen_seq = seq;
    return IPC_OK;
}

/* ===================== 时间戳校验 ===================== */

/*
 * validate_timestamp:
 *   校验消息时间戳是否在有效窗口内（±5 秒）。
 *
 * 参数：
 *   msg_ts - 消息中的时间戳（纳秒）
 *   now_ns - 当前时间戳（纳秒，通常来自 CLOCK_MONOTONIC）
 *
 * 规则：
 *   - |now_ns - msg_ts| <= IPC_TIMESTAMP_VALID_WINDOW_NS (5s)
 */
ipc_status_t validate_timestamp(uint64_t msg_ts, uint64_t now_ns) {
    uint64_t diff;

    /* 计算绝对差值 */
    if (now_ns >= msg_ts) {
        diff = now_ns - msg_ts;
    } else {
        diff = msg_ts - now_ns;
    }

    /* 检查是否超出 5 秒窗口 */
    if (diff > IPC_TIMESTAMP_VALID_WINDOW_NS) {
        return IPC_ERR_TIMESTAMP_EXPIRED;
    }

    return IPC_OK;
}

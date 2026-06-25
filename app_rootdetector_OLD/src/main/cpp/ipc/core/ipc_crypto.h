/*
 * ipc_crypto.h - IPC 加密层接口
 *
 * 提供 AES-GCM 加密/解密功能，绑定序列号+时间戳作为 AAD。
 * 所有 IPC 消息必须经过此层加密后才能发送。
 */

#ifndef IPC_CRYPTO_H
#define IPC_CRYPTO_H

#include "ipc_protocol.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 加密上下文 ===================== */

/*
 * IPC 加密上下文（每个连接维护一个）
 * 包含会话密钥、序列号状态、时间戳校验等。
 */
typedef struct {
    uint8_t  session_key[32];        /* AES-256 会话密钥 */
    uint64_t last_seen_seq;          /* 最后见过的序列号（防重放） */
    uint64_t last_timestamp_ns;      /* 最后见过的时间戳 */
    int      is_initialized;         /* 是否已初始化 */
} ipc_crypto_ctx_t;

/* ===================== 加密 API ===================== */

/*
 * ipc_crypto_init:
 *   初始化加密上下文，设置会话密钥。
 *
 * 参数：
 *   ctx        - 加密上下文
 *   key        - 32 字节 AES-256 密钥
 *   key_len    - 密钥长度（必须为 32）
 */
__attribute__((visibility("default")))
ipc_status_t ipc_crypto_init(ipc_crypto_ctx_t *ctx,
                              const uint8_t *key,
                              size_t key_len);

/*
 * ipc_crypto_cleanup:
 *   安全清理加密上下文（清零敏感数据）。
 */
__attribute__((visibility("default")))
void ipc_crypto_cleanup(ipc_crypto_ctx_t *ctx);

/*
 * encrypt_message:
 *   对 IPC 消息进行 AES-GCM 加密。
 *
 *   输入：msg->payload 为明文载荷
 *   输出：msg->payload 被替换为密文，msg->header.tag 填充认证标签，
 *         msg->header.aad 填充附加认证数据，msg->is_encrypted = 1
 *
 *   加密流程：
 *   1. 构造 AAD = msg_type(4) || seq_low(4) || seq_high(4) || ts_low(4)
 *   2. 生成 IV/Nonce（12 字节，从序列号+时间戳派生）
 *   3. 调用 aes256_gcm_encrypt() 加密
 *   4. 将 tag 拷贝到 msg->header.tag
 *
 * 参数：
 *   msg        - 消息容器（明文输入，密文输出）
 *   ctx        - 加密上下文（包含会话密钥）
 *
 * 返回：
 *   IPC_OK             - 成功
 *   IPC_ERR_INVALID_ARG - 参数错误
 *   IPC_ERR_CRYPTO_ENCRYPT - 加密失败
 */
__attribute__((visibility("default")))
ipc_status_t encrypt_message(ipc_message_t *msg,
                              ipc_crypto_ctx_t *ctx);

/*
 * decrypt_message:
 *   对 IPC 消息进行 AES-GCM 解密和校验。
 *
 *   输入：msg->payload 为密文，msg->header.tag 为认证标签
 *   输出：msg->payload 被替换为明文，msg->is_encrypted = 0
 *
 *   解密流程：
 *   1. 校验序列号（防重放）
 *   2. 校验时间戳（±5 秒窗口）
 *   3. 从 msg->header 构造 AAD
 *   4. 派生 IV/Nonce
 *   5. 调用 aes256_gcm_decrypt() 解密并校验 tag
 *   6. 若校验失败，触发安全告警
 *
 * 参数：
 *   msg        - 消息容器（密文输入，明文输出）
 *   ctx        - 加密上下文
 *   now_ns     - 当前时间戳（纳秒，用于校验）
 *
 * 返回：
 *   IPC_OK                 - 成功
 *   IPC_ERR_INVALID_ARG    - 参数错误
 *   IPC_ERR_REPLAY_SEQ_OLD - 序列号过旧（重放攻击）
 *   IPC_ERR_REPLAY_SEQ_DUP - 序列号重复（重放攻击）
 *   IPC_ERR_TIMESTAMP_EXPIRED - 时间戳过期
 *   IPC_ERR_CRYPTO_DECRYPT - 解密失败
 *   IPC_ERR_TAG_MISMATCH   - 认证标签不匹配（篡改）
 */
__attribute__((visibility("default")))
ipc_status_t decrypt_message(ipc_message_t *msg,
                              ipc_crypto_ctx_t *ctx,
                              uint64_t now_ns);

/*
 * derive_nonce:
 *   从序列号和时间戳派生 12 字节 Nonce/IV。
 *   Nonce = seq(8) XOR ts(8) 的低 12 字节
 *
 * 参数：
 *   seq        - 64 位序列号
 *   ts         - 64 位时间戳
 *   out_nonce  - 输出 12 字节 Nonce
 */
void derive_nonce(uint64_t seq, uint64_t ts, uint8_t out_nonce[IPC_NONCE_LEN]);

/*
 * ipc_crypto_trigger_alert:
 *   触发安全告警（当解密/校验失败时调用）。
 *   构造 SecurityAlert 消息并发送给对端。
 *
 * 参数：
 *   alert_type - 告警类型（ipc_alert_type_t）
 *   severity   - 严重级别（ipc_severity_t）
 *   related_seq - 关联的消息序列号
 *   detail     - 详情字符串
 *   ctx        - 加密上下文（用于加密告警消息）
 *   send_fd    - 发送文件描述符
 */
void ipc_crypto_trigger_alert(ipc_alert_type_t alert_type,
                               ipc_severity_t severity,
                               uint64_t related_seq,
                               const char *detail,
                               ipc_crypto_ctx_t *ctx,
                               int send_fd);

#ifdef __cplusplus
}
#endif

#endif /* IPC_CRYPTO_H */

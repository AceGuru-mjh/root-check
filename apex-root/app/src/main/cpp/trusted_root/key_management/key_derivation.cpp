#include "key_derivation.h"
#include "crypto/crypto_primitives.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cinttypes>
#include <mutex>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// FIX-P1-CPP (v1.1.2): 本文件多处问题已修复 (P1-C12):
//   1. initialize_device_key 用 std::call_once + std::once_flag 做线程安全初始化
//      (原 g_initialized 全局变量无锁, 多线程并发会重复生成密钥并产生 data race)
//   2. 密钥持久化到 /data/data/com.apex.root/files/device_key.bin
//      (原代码每次进程启动都生成新密钥, 上次签名的报告这次无法验证)
//      TODO: 加密 key 应从 Android Keystore 派生, 当前用固定 hardcoded key 占位
//   3. store_encrypted 加 O_TRUNC (0x200), 避免旧文件更长时残留
//   4. load_encrypted 用 bs_lseek 动态获取文件大小, 用 std::vector 动态分配
//      (原用固定 4096 字节栈缓冲区, 密文超长被截断)

namespace apex {
namespace key {

// 设备密钥持久化路径 — app 内部存储 (与 Context.filesDir 对应)
// 注意: 由 daemon_main.cpp 在 JNI_OnLoad 或 trusted_daemon 启动时初始化
static constexpr const char* DEVICE_KEY_PATH =
    "/data/data/com.apex.root/files/device_key.bin";

// 持久化加密用的固定 key — TODO: 应改为从 Android Keystore 派生
// (需 Kotlin 侧配合通过 JNI 把 Keystore 派生的 key 传下来)
// 当前用固定 key 仅防止密钥文件被直接读取, 不能抵御逆向
static constexpr uint8_t PERSISTENCE_KEY[32] = {
    0xA1, 0x9E, 0xC4, 0x67, 0x2B, 0x8F, 0x4D, 0x1A,
    0xE3, 0x75, 0x9C, 0x42, 0xB8, 0x6D, 0x21, 0xF0,
    0x5A, 0xE7, 0x83, 0x19, 0xCC, 0x40, 0x9B, 0x2E,
    0xD1, 0x66, 0x84, 0xA3, 0x52, 0xF9, 0x07, 0xBD
};

static DeviceKeyPair g_device_key;
static std::once_flag g_init_once;

// v1.1.3 P2-S4: 密钥分离 — 从 Dilithium secret_key 派生独立的 AES 加密密钥
// ─────────────────────────────────────────────────────────────
// 此前 store_encrypted/load_encrypted 直接用 g_device_key.secret_key 前 32 字节
// 作为 AES-256 key, 违反密钥分离原则 (签名密钥 ≠ 加密密钥):
//   - 一旦 AES key 泄露, 攻击者可伪造签名 (因 AES key 是签名 key 的子集)
//   - Dilithium secret_key 字节分布不适合直接做 AES key
// 修复: 用 HMAC-SHA3-512(secret_key, "APEX-ENC-V1") 派生独立 enc_key (取前 32 字节)
// HKDF 风格: PRK = HMAC(secret_key, info), enc_key = PRK[0..31]
// 这样 enc_key 与 secret_key 密码学独立, 泄露一方不会危害另一方。
// ─────────────────────────────────────────────────────────────
static void derive_enc_key(const uint8_t* secret_key, size_t key_len, uint8_t out[32]) {
    static const uint8_t info[] = "APEX-ENC-V1";  // 不含末尾 \0
    auto mac = crypto::hmac_sha3_512(secret_key, key_len,
                                       info, sizeof(info) - 1);
    std::memcpy(out, mac.data(), 32);
}

// 内部: 从持久化文件加载密钥 (用 PERSISTENCE_KEY 解密)
// 返回 true 表示成功加载, false 表示文件不存在或解密失败
static bool load_persisted_key(DeviceKeyPair& out) {
    auto fd = bs_openat(-100, DEVICE_KEY_PATH, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;

    // FIX-P1-CPP: 用 bs_lseek 动态获取文件大小, 不再用固定 4096 栈缓冲区
    int64_t file_size = bs_lseek((int)fd, 0, SEEK_END);
    if (file_size <= 0) {
        bs_close(fd);
        return false;
    }
    bs_lseek((int)fd, 0, SEEK_SET);

    // 限制最大读取 1MB, 防止异常大文件耗尽内存
    if (file_size > 1024 * 1024) {
        bs_close(fd);
        return false;
    }

    std::vector<uint8_t> ct((size_t)file_size);
    int64_t total_read = 0;
    while (total_read < file_size) {
        int64_t r = bs_read(fd, ct.data() + total_read, (size_t)(file_size - total_read));
        if (r <= 0) break;
        total_read += r;
    }
    bs_close(fd);
    if (total_read != file_size) return false;

    // 用 PERSISTENCE_KEY 解密
    auto pt = crypto::aes256_gcm_decrypt(ct.data(), ct.size(),
                                          PERSISTENCE_KEY, 32);
    if (pt.size() < 8) return false;

    // 反序列化: [4字节 public_key_len][public_key][4字节 secret_key_len][secret_key]
    if (pt.size() < 8) return false;
    uint32_t pub_len = 0, sec_len = 0;
    std::memcpy(&pub_len, pt.data(), 4);
    std::memcpy(&sec_len, pt.data() + 4 + pub_len, 4);
    if (8ULL + pub_len + sec_len != pt.size()) return false;

    out.public_key.assign(pt.data() + 4, pt.data() + 4 + pub_len);
    out.secret_key.assign(pt.data() + 8 + pub_len,
                           pt.data() + 8 + pub_len + sec_len);
    return !out.public_key.empty() && !out.secret_key.empty();
}

// 内部: 持久化密钥到文件 (用 PERSISTENCE_KEY 加密)
static bool persist_key(const DeviceKeyPair& kp) {
    // 序列化: [4字节 pub_len][pub][4字节 sec_len][sec]
    std::vector<uint8_t> pt;
    pt.reserve(8 + kp.public_key.size() + kp.secret_key.size());
    uint32_t pub_len = (uint32_t)kp.public_key.size();
    uint32_t sec_len = (uint32_t)kp.secret_key.size();
    pt.insert(pt.end(), (uint8_t*)&pub_len, (uint8_t*)&pub_len + 4);
    pt.insert(pt.end(), kp.public_key.begin(), kp.public_key.end());
    pt.insert(pt.end(), (uint8_t*)&sec_len, (uint8_t*)&sec_len + 4);
    pt.insert(pt.end(), kp.secret_key.begin(), kp.secret_key.end());

    auto ct = crypto::aes256_gcm_encrypt(pt.data(), pt.size(),
                                          PERSISTENCE_KEY, 32);
    if (ct.empty()) return false;

    // FIX-P1-CPP: 加 O_TRUNC (0x200), 旧文件更长时不会残留 (P1-C12.3)
    // 0x41 (O_CREAT|O_RDWR) | 0x200 (O_TRUNC) = 0x241
    // 同时加 O_CLOEXEC (0x80000) 防止 fork 后子进程继承 fd
    auto fd = bs_openat(-100, DEVICE_KEY_PATH,
                         0x41 | 0x200 | 0x80000, 0600);
    if (fd < 0) return false;
    auto written = bs_write(fd, ct.data(), ct.size());
    bs_close(fd);
    return written == static_cast<int64_t>(ct.size());
}

// call_once 内部函数: 实际生成或加载密钥
static void do_initialize() {
    // 先尝试从持久化文件加载
    DeviceKeyPair loaded;
    if (load_persisted_key(loaded)) {
        g_device_key = std::move(loaded);
        return;
    }

    // 加载失败 — 生成新密钥并持久化
    auto kp = crypto::generate_dilithium_keypair();
    g_device_key.public_key = kp.public_key;
    g_device_key.secret_key = kp.secret_key;

    if (g_device_key.public_key.empty() || g_device_key.secret_key.empty()) {
        return;  // g_initialized 通过 once_flag 隐式管理
    }

    // 持久化 (失败不影响初始化, 下次启动会重新生成)
    persist_key(g_device_key);
}

bool initialize_device_key() {
    bool ok = false;
    std::call_once(g_init_once, [&ok]() {
        do_initialize();
        ok = !g_device_key.public_key.empty() && !g_device_key.secret_key.empty();
    });
    return ok;
}

DeviceKeyPair get_device_keypair() {
    // 注意: 调用方应先调用 initialize_device_key()。
    // 这里不强制 call_once 以避免在 read-only 路径上加锁, 但返回可能为空。
    return g_device_key;
}

SessionKey generate_session_key() {
    SessionKey sk;
    sk.key.resize(32);
    // Get random bytes from kernel
    for (size_t i = 0; i < 32; i += 8) {
        uint64_t r = bs_get_random();
        std::memcpy(sk.key.data() + i, &r, (32 - i < 8) ? (32 - i) : 8);
    }
    sk.created_at = bs_clock_ns();
    return sk;
}

bool store_encrypted(const uint8_t* data, size_t len, const char* name) {
    // v1.1.3 P2-S4: 用 derive_enc_key 派生独立 enc_key, 不直接用 secret_key 前 32 字节
    // 密钥分离: 签名密钥 ≠ 加密密钥, 泄露一方不会危害另一方
    if (g_device_key.secret_key.size() < 32) return false;
    uint8_t enc_key[32];
    derive_enc_key(g_device_key.secret_key.data(),
                    g_device_key.secret_key.size(), enc_key);
    auto ct = crypto::aes256_gcm_encrypt(data, len, enc_key, 32);
    // 用完立即清零, 防止内存残留
    std::memset(enc_key, 0, sizeof(enc_key));
    if (ct.empty()) return false;
    // FIX-P1-CPP (v1.1.2): 加 O_TRUNC (0x200) 防止旧文件更长时残留 (P1-C12.3)
    // 0x41 (O_CREAT|O_RDWR) | 0x200 (O_TRUNC) = 0x241
    // 同时加 O_CLOEXEC (0x80000)
    auto fd = bs_openat(-100, name, 0x41 | 0x200 | 0x80000, 0600);
    if (fd < 0) return false;
    auto written = bs_write(fd, ct.data(), ct.size());
    bs_close(fd);
    return written == static_cast<int64_t>(ct.size());
}

bool load_encrypted(uint8_t* out, size_t* len, const char* name) {
    auto fd = bs_openat(-100, name, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;

    // FIX-P1-CPP (v1.1.2): 用 bs_lseek 动态获取文件大小, 用 std::vector 动态分配
    // (原代码用固定 4096 字节栈缓冲区, 密文超长被截断 — P1-C12.4)
    int64_t file_size = bs_lseek((int)fd, 0, SEEK_END);
    if (file_size <= 0) {
        bs_close(fd);
        return false;
    }
    bs_lseek((int)fd, 0, SEEK_SET);

    // 限制最大读取 1MB, 防止异常大文件耗尽内存
    if (file_size > 1024 * 1024) {
        bs_close(fd);
        return false;
    }

    std::vector<uint8_t> buf((size_t)file_size);
    int64_t total_read = 0;
    while (total_read < file_size) {
        int64_t r = bs_read(fd, buf.data() + total_read,
                             (size_t)(file_size - total_read));
        if (r <= 0) break;
        total_read += r;
    }
    bs_close(fd);
    if (total_read != file_size) return false;

    // v1.1.3 P2-S4: 用 derive_enc_key 派生独立 enc_key, 与 store_encrypted 对应
    if (g_device_key.secret_key.size() < 32) return false;
    uint8_t enc_key[32];
    derive_enc_key(g_device_key.secret_key.data(),
                    g_device_key.secret_key.size(), enc_key);
    auto pt = crypto::aes256_gcm_decrypt(
        buf.data(), (size_t)total_read, enc_key, 32);
    std::memset(enc_key, 0, sizeof(enc_key));
    if (pt.empty()) return false;

    if (*len < pt.size()) return false;
    std::memcpy(out, pt.data(), pt.size());
    *len = pt.size();
    return true;
}

} // namespace key
} // namespace apex

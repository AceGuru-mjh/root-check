#include "oqs_signature.h"
#include "crypto_primitives.h"
#include <cstring>
#include <cctype>
#include <android/log.h>

#define LOG_TAG "APEX-OQS"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef APEX_HAVE_LIBOQS
#include <oqs/oqs.h>
#endif

namespace apex {
namespace crypto {

OqsSignature& OqsSignature::getInstance() {
    static OqsSignature instance;
    return instance;
}

OqsSignature::OqsSignature() {
#ifdef APEX_HAVE_LIBOQS
    sig = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!sig) {
        LOGE("OQS_SIG_new failed for %s", OQS_SIG_alg_dilithium_3);
        return;
    }
    OQS_SIG* s = static_cast<OQS_SIG*>(sig);
    pubKeySize = s->length_public_key;
    secKeySize = s->length_secret_key;
    sigSize = s->length_signature;
    available = true;
    LOGD("OQS %s initialized: pk=%zu sk=%zu sig=%zu",
         OQS_SIG_alg_dilithium_3, pubKeySize, secKeySize, sigSize);
#else
    // FIX-CPP P0-C1: 未编译 liboqs 时 available 必须为 false。
    // 原实现 available=true 且 sign/verify 走 HMAC-SHA3-512 fallback ——
    // fallback 的 verify 用 public_key 派生 HMAC 密钥,任何人都能伪造签名。
    // 这样能让上层 (signing_center / downloader) 正确走“无后量子签名”路径,
    // 而不是误以为签名可用。
    available = false;
    pubKeySize = 0;
    secKeySize = 0;
    sigSize = 0;
    LOGE("OQS built without liboqs — post-quantum signature DISABLED "
         "(fallback removed for security: HMAC fallback was forgeable)");
#endif
}

OqsSignature::~OqsSignature() {
#ifdef APEX_HAVE_LIBOQS
    if (sig) {
        OQS_SIG_free(static_cast<OQS_SIG*>(sig));
        sig = nullptr;
    }
#endif
}

bool OqsSignature::generateKeyPair(std::vector<uint8_t>& publicKey,
                                    std::vector<uint8_t>& privateKey) {
#ifdef APEX_HAVE_LIBOQS
    std::lock_guard<std::mutex> lock(mtx);
    if (!available) return false;
    OQS_SIG* s = static_cast<OQS_SIG*>(sig);
    publicKey.resize(pubKeySize);
    privateKey.resize(secKeySize);
    OQS_STATUS status = OQS_SIG_keypair(s, publicKey.data(), privateKey.data());
    if (status != OQS_SUCCESS) {
        LOGE("Key generation failed: %d", status);
        return false;
    }
    return true;
#else
    // FIX-CPP P0-C1: 无 liboqs 时直接拒绝生成密钥对。
    // 原实现走 HMAC-SHA3-512 fallback,生成的“密钥对”只是随机 64 字节 + 其 SHA3-512 哈希,
    // 不能作为后量子签名使用。
    (void)publicKey;
    (void)privateKey;
    LOGE("generateKeyPair: liboqs unavailable, refused");
    return false;
#endif
}

std::vector<uint8_t> OqsSignature::sign(const std::vector<uint8_t>& data,
                                          const std::vector<uint8_t>& privateKey) {
#ifdef APEX_HAVE_LIBOQS
    std::lock_guard<std::mutex> lock(mtx);
    if (!available) return {};
    OQS_SIG* s = static_cast<OQS_SIG*>(sig);
    if (privateKey.size() != secKeySize) return {};

    std::vector<uint8_t> signature(sigSize);
    size_t sigLen = 0;
    OQS_STATUS status = OQS_SIG_sign(s, signature.data(), &sigLen,
        data.data(), data.size(), privateKey.data());
    if (status != OQS_SUCCESS) {
        LOGE("Sign failed: %d", status);
        return {};
    }
    signature.resize(sigLen);
    return signature;
#else
    // FIX-CPP P0-C1: 无 liboqs 时签名直接返回空。
    // 原实现走 dilithium_sign (HMAC-SHA3-512 fallback),其验证逻辑可被任何
    // 拥有 public_key 的一方伪造,不提供真实不可抵赖性。
    (void)data;
    (void)privateKey;
    LOGE("sign: liboqs unavailable, refused (fallback removed for security)");
    return {};
#endif
}

bool OqsSignature::verify(const std::vector<uint8_t>& data,
                           const std::vector<uint8_t>& signature,
                           const std::vector<uint8_t>& publicKey) {
#ifdef APEX_HAVE_LIBOQS
    std::lock_guard<std::mutex> lock(mtx);
    if (!available) return false;
    OQS_SIG* s = static_cast<OQS_SIG*>(sig);
    if (signature.size() != sigSize || publicKey.size() != pubKeySize) return false;

    OQS_STATUS status = OQS_SIG_verify(s, data.data(), data.size(),
        signature.data(), signature.size(), publicKey.data());
    return status == OQS_SUCCESS;
#else
    // FIX-CPP P0-C1: 无 liboqs 时验证直接返回 false。
    // 原实现走 dilithium_verify fallback,该 fallback 用 public_key 派生 HMAC 密钥
    // 来“验证”签名 —— 任何拿到 public_key 的人都能伪造合法签名,完全无效。
    (void)data;
    (void)signature;
    (void)publicKey;
    LOGE("verify: liboqs unavailable, refused (fallback removed for security)");
    return false;
#endif
}

// ─── Base64 (RFC 4648) ─────────────────────────────────────

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
static const std::string B64URL_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

std::string OqsSignature::base64Encode(const std::vector<uint8_t>& data, bool urlSafe) {
    const auto& chars = urlSafe ? B64URL_CHARS : B64_CHARS;
    std::string ret;
    int i = 0;
    uint8_t a3[3], a4[4];
    for (auto byte : data) {
        a3[i++] = byte;
        if (i == 3) {
            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
            a4[3] = a3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += chars[a4[i]];
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 3; j++) a3[j] = '\0';
        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;
        for (int j = 0; j < i + 1; j++) ret += chars[a4[j]];
        if (!urlSafe) while (i++ < 3) ret += '=';
    }
    return ret;
}

std::vector<uint8_t> OqsSignature::base64Decode(const std::string& encoded, bool urlSafe) {
    const auto& chars = urlSafe ? B64URL_CHARS : B64_CHARS;
    std::vector<uint8_t> ret;
    int i = 0, pos = 0;
    uint8_t a4[4], a3[3];
    while (pos < (int)encoded.size() && encoded[pos] != '=') {
        auto c = encoded[pos];
        if (!(std::isalnum(c) || c == '+' || c == '/' || c == '-' || c == '_')) {
            pos++;
            continue;
        }
        a4[i++] = (uint8_t)chars.find(c);
        if (i == 4) {
            a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
            a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
            a3[2] = ((a4[2] & 0x03) << 6) | a4[3];
            for (i = 0; i < 3; i++) ret.push_back(a3[i]);
            i = 0;
        }
        pos++;
    }
    if (i) {
        a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
        a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
        for (int j = 0; j < i - 1; j++) ret.push_back(a3[j]);
    }
    return ret;
}

std::vector<uint8_t> OqsSignature::stringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string OqsSignature::bytesToString(const std::vector<uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::array<uint8_t, 64> OqsSignature::hash512(const uint8_t* data, size_t len) {
    return sha3_512(data, len);
}

} // namespace crypto
} // namespace apex

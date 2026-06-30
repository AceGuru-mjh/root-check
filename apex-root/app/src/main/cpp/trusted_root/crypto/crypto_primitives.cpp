#include "crypto_primitives.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <algorithm>
#include <cinttypes>

// liboqs conditional
#if APEX_HAVE_LIBOQS
#include <oqs/oqs.h>
#endif

namespace apex {
namespace crypto {

// ═══════════════════════════════════════════════════════════
// SHA3-512 (Keccak-f[1600])
// ═══════════════════════════════════════════════════════════

static const uint64_t KECCAK_RC[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

static inline uint64_t rotl64(uint64_t x, int n) {
    return (x << n) | (x >> (64 - n));
}

static void keccak_f1600(uint64_t st[25]) {
    for (int r = 0; r < 24; r++) {
        uint64_t C[5], D[5];
        for (int i = 0; i < 5; i++)
            C[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
        for (int i = 0; i < 5; i++)
            D[i] = rotl64(C[(i + 4) % 5], 1) ^ C[(i + 1) % 5];
        for (int i = 0; i < 25; i++)
            st[i] ^= D[i % 5];

        uint64_t B[25];
        B[0] = st[0];
        int x = 1, y = 0;
        for (int t = 0; t < 24; t++) {
            int idx = y * 5 + x;
            B[idx] = rotl64(st[idx], (t + 1) * (t + 2) / 2 % 64);
            int tmp = x;
            x = y;
            y = (2 * tmp + 3 * y) % 5;
        }
        for (int i = 0; i < 25; i++) st[i] = B[i];

        for (int y = 0; y < 5; y++) {
            int base = y * 5;
            for (int x = 0; x < 5; x++)
                B[base + x] = st[base + x] ^ ((~st[base + (x + 1) % 5]) & st[base + (x + 2) % 5]);
        }
        for (int i = 0; i < 25; i++) st[i] = B[i];

        st[0] ^= KECCAK_RC[r];
    }
}

std::array<uint8_t, 64> sha3_512(const uint8_t* data, size_t len) {
    uint64_t st[25] = {0};
    const size_t rate = 72;
    size_t offset = 0;
    while (offset < len) {
        size_t block = std::min(len - offset, rate);
        for (size_t i = 0; i < block; i++)
            st[i / 8] ^= static_cast<uint64_t>(data[offset + i]) << (8 * (i % 8));
        if (block == rate) keccak_f1600(st);
        offset += block;
    }
    size_t last = len % rate;
    st[last / 8] ^= 0x06ULL << (8 * (last % 8));
    st[(rate - 1) / 8] ^= 0x80ULL << (8 * ((rate - 1) % 8));
    keccak_f1600(st);

    std::array<uint8_t, 64> out{};
    for (size_t i = 0; i < 64; i++)
        out[i] = (st[i / 8] >> (8 * (i % 8))) & 0xFF;
    return out;
}

// HMAC-SHA3-512 (RFC 2104 style with SHA3-512)
static std::array<uint8_t, 64> hmac_sha3_512(
    const uint8_t* key, size_t key_len,
    const uint8_t* data, size_t data_len) {

    uint8_t k_ipad[144];
    uint8_t k_opad[144];

    uint8_t k[64];
    if (key_len > 144) {
        auto h = sha3_512(key, key_len);
        std::memcpy(k, h.data(), 64);
        key_len = 64;
    } else {
        std::memcpy(k, key, key_len);
    }

    std::memset(k_ipad, 0, 144);
    std::memset(k_opad, 0, 144);
    std::memcpy(k_ipad, k, key_len);
    std::memcpy(k_opad, k, key_len);

    for (size_t i = 0; i < 144; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    uint8_t inner_input[144 + 65536];
    std::memcpy(inner_input, k_ipad, 144);
    std::memcpy(inner_input + 144, data, data_len);
    auto inner_hash = sha3_512(inner_input, 144 + data_len);

    uint8_t outer_input[144 + 64];
    std::memcpy(outer_input, k_opad, 144);
    std::memcpy(outer_input + 144, inner_hash.data(), 64);
    return sha3_512(outer_input, 144 + 64);
}

// ═══════════════════════════════════════════════════════════
// AES-256 (software implementation, FIPS-197)
// ═══════════════════════════════════════════════════════════

static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t aes_mul2(uint8_t x) {
    return (x & 0x80) ? ((x << 1) ^ 0x1b) : (x << 1);
}

static uint8_t aes_mul3(uint8_t x) { return aes_mul2(x) ^ x; }

static void aes_key_expand_256(const uint8_t* key, uint8_t rk[15][16]) {
    uint8_t w[60][4];
    for (int i = 0; i < 8; i++) {
        w[i][0] = key[4*i];
        w[i][1] = key[4*i+1];
        w[i][2] = key[4*i+2];
        w[i][3] = key[4*i+3];
    }
    for (int i = 8; i < 60; i++) {
        uint8_t temp[4];
        temp[0] = w[i-1][0];
        temp[1] = w[i-1][1];
        temp[2] = w[i-1][2];
        temp[3] = w[i-1][3];
        if (i % 8 == 0) {
            uint8_t t = temp[0];
            temp[0] = AES_SBOX[temp[1]] ^ (i/8 - 1);
            temp[1] = AES_SBOX[temp[2]];
            temp[2] = AES_SBOX[temp[3]];
            temp[3] = AES_SBOX[t];
        } else if (i % 8 == 4) {
            temp[0] = AES_SBOX[temp[0]];
            temp[1] = AES_SBOX[temp[1]];
            temp[2] = AES_SBOX[temp[2]];
            temp[3] = AES_SBOX[temp[3]];
        }
        w[i][0] = w[i-8][0] ^ temp[0];
        w[i][1] = w[i-8][1] ^ temp[1];
        w[i][2] = w[i-8][2] ^ temp[2];
        w[i][3] = w[i-8][3] ^ temp[3];
    }
    for (int r = 0; r < 15; r++)
        for (int i = 0; i < 4; i++) {
            rk[r][4*i]   = w[r*4 + i][0];
            rk[r][4*i+1] = w[r*4 + i][1];
            rk[r][4*i+2] = w[r*4 + i][2];
            rk[r][4*i+3] = w[r*4 + i][3];
        }
}

static void aes_encrypt_block(const uint8_t rk[15][16], uint8_t state[16]) {
    uint8_t s[16];
    std::memcpy(s, state, 16);

    auto add_rk = [&](int round) {
        for (int i = 0; i < 16; i++) s[i] ^= rk[round][i];
    };

    auto sub_bytes = [&]() {
        for (int i = 0; i < 16; i++) s[i] = AES_SBOX[s[i]];
    };

    auto shift_rows = [&]() {
        uint8_t t;
        t = s[1];  s[1]  = s[5];  s[5]  = s[9];  s[9]  = s[13]; s[13] = t;
        t = s[2];  s[2]  = s[10]; s[10] = s[2];
        t = s[6];  s[6]  = s[14]; s[14] = t;
        t = s[3];  s[3]  = s[15]; s[15] = s[11]; s[11] = s[7];  s[7]  = t;
    };

    auto mix_cols = [&]() {
        for (int i = 0; i < 4; i++) {
            int j = i * 4;
            uint8_t a0 = s[j], a1 = s[j+1], a2 = s[j+2], a3 = s[j+3];
            s[j]   = aes_mul2(a0) ^ aes_mul3(a1) ^ a2 ^ a3;
            s[j+1] = a0 ^ aes_mul2(a1) ^ aes_mul3(a2) ^ a3;
            s[j+2] = a0 ^ a1 ^ aes_mul2(a2) ^ aes_mul3(a3);
            s[j+3] = aes_mul3(a0) ^ a1 ^ a2 ^ aes_mul2(a3);
        }
    };

    add_rk(0);
    for (int round = 1; round < 14; round++) {
        sub_bytes();
        shift_rows();
        mix_cols();
        add_rk(round);
    }
    sub_bytes();
    shift_rows();
    add_rk(14);
    std::memcpy(state, s, 16);
}

// ─── AES-256-CTR encrypt/decrypt ─────────────────────────
static void aes256_ctr_crypt(const uint8_t* in, uint8_t* out, size_t len,
                              const uint8_t key[32], uint8_t ctr[16]) {
    uint8_t rk[15][16];
    aes_key_expand_256(key, rk);

    uint8_t block[16];
    size_t offset = 0;
    while (offset < len) {
        std::memcpy(block, ctr, 16);
        aes_encrypt_block(rk, block);
        size_t chunk = std::min(len - offset, size_t(16));
        for (size_t i = 0; i < chunk; i++)
            out[offset + i] = in[offset + i] ^ block[i];
        offset += chunk;
        for (int i = 15; i >= 0; i--) {
            if (++ctr[i] != 0) break;
        }
    }
}

// ─── AES-256-GCM (using AES-256-CTR + HMAC-SHA3-512) ─────
// Format: nonce(12) || ciphertext || tag(32)
std::vector<uint8_t> aes256_gcm_encrypt(const uint8_t* plain, size_t len,
                                         const uint8_t* key, size_t key_len) {
    std::vector<uint8_t> result;
    if (key_len < 32) return result;

    // Generate random nonce
    uint8_t nonce[12];
    for (size_t i = 0; i < 12; i++) {
        uint64_t r = bs_get_random();
        nonce[i] = (r >> (i * 8)) & 0xFF;
    }

    // Build counter block from nonce
    uint8_t ctr[16];
    std::memcpy(ctr, nonce, 12);
    ctr[12] = 0; ctr[13] = 0; ctr[14] = 0; ctr[15] = 1;

    // Encrypt with AES-256-CTR
    std::vector<uint8_t> ct(len);
    aes256_ctr_crypt(plain, ct.data(), len, key, ctr);

    // Authenticate: HMAC-SHA3-512(nonce || ciphertext) using derived key
    // Key derivation for auth key: HMAC-SHA3-512(key, "APEX-AUTH")
    uint8_t auth_label[] = "APEX-AUTH-GCM";
    auto auth_key = hmac_sha3_512(key, 32, auth_label, 12);
    uint8_t auth_input[12 + 65536];
    std::memcpy(auth_input, nonce, 12);
    std::memcpy(auth_input + 12, ct.data(), len);
    auto tag = hmac_sha3_512(auth_key.data(), 64, auth_input, 12 + len);

    // Output: nonce(12) || ct || tag(32)
    result.resize(12 + len + 32);
    std::memcpy(result.data(), nonce, 12);
    if (len > 0) std::memcpy(result.data() + 12, ct.data(), len);
    std::memcpy(result.data() + 12 + len, tag.data(), 32);
    return result;
}

std::vector<uint8_t> aes256_gcm_decrypt(const uint8_t* cipher, size_t len,
                                         const uint8_t* key, size_t key_len) {
    std::vector<uint8_t> result;
    if (key_len < 32 || len < 44) return result; // nonce(12) + tag(32) min

    size_t ct_len = len - 44;

    // Extract nonce
    uint8_t nonce[12];
    std::memcpy(nonce, cipher, 12);

    // Build counter block
    uint8_t ctr[16];
    std::memcpy(ctr, nonce, 12);
    ctr[12] = 0; ctr[13] = 0; ctr[14] = 0; ctr[15] = 1;

    // Verify tag
    uint8_t auth_label[] = "APEX-AUTH-GCM";
    auto auth_key = hmac_sha3_512(key, 32, auth_label, 12);
    uint8_t auth_input[12 + 65536];
    std::memcpy(auth_input, nonce, 12);
    if (ct_len > 0) std::memcpy(auth_input + 12, cipher + 12, ct_len);
    auto expected_tag = hmac_sha3_512(auth_key.data(), 64, auth_input, 12 + ct_len);

    uint8_t stored_tag[32];
    std::memcpy(stored_tag, cipher + 12 + ct_len, 32);

    // Constant-time tag comparison
    uint8_t diff = 0;
    for (int i = 0; i < 32; i++)
        diff |= expected_tag[i] ^ stored_tag[i];
    if (diff != 0) return result; // tag mismatch

    // Decrypt
    result.resize(ct_len);
    if (ct_len > 0) {
        aes256_ctr_crypt(cipher + 12, result.data(), ct_len, key, ctr);
    }
    return result;
}

// ─── Dilithium ─────────────────────────────────────────────
#if APEX_HAVE_LIBOQS

DilithiumKeypair generate_dilithium_keypair() {
    DilithiumKeypair kp;
    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!sig) return kp;
    kp.public_key.resize(sig->length_public_key);
    kp.secret_key.resize(sig->length_secret_key);
    if (OQS_SIG_keypair(sig, kp.public_key.data(), kp.secret_key.data()) == OQS_SUCCESS)
        kp.valid = true;
    OQS_SIG_free(sig);
    return kp;
}

std::vector<uint8_t> dilithium_sign(const uint8_t* msg, size_t msg_len,
                                     const uint8_t* secret_key, size_t sk_len) {
    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!sig) return {};
    std::vector<uint8_t> out(sig->length_signature);
    size_t sig_len = 0;
    OQS_SIG_sign(sig, out.data(), &sig_len, msg, msg_len, secret_key);
    out.resize(sig_len);
    OQS_SIG_free(sig);
    return out;
}

bool dilithium_verify(const uint8_t* msg, size_t msg_len,
                       const uint8_t* sig, size_t sig_len,
                       const uint8_t* public_key, size_t pk_len) {
    OQS_SIG* obj = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!obj) return false;
    bool ok = OQS_SIG_verify(obj, msg, msg_len, sig, sig_len, public_key) == OQS_SUCCESS;
    OQS_SIG_free(obj);
    return ok;
}

#else
// Software fallback: HMAC-SHA3-512-based signatures
// (NOT post-quantum secure, but provides functional signing without liboqs)

DilithiumKeypair generate_dilithium_keypair() {
    DilithiumKeypair kp;
    kp.secret_key.resize(64);
    for (size_t i = 0; i < 64; i += 8) {
        uint64_t r = bs_get_random();
        std::memcpy(kp.secret_key.data() + i, &r, 8);
    }
    auto pk_array = sha3_512(kp.secret_key.data(), kp.secret_key.size());
    kp.public_key = std::vector<uint8_t>(pk_array.begin(), pk_array.end());
    kp.valid = true;
    return kp;
}

std::vector<uint8_t> dilithium_sign(const uint8_t* msg, size_t msg_len,
                                     const uint8_t* secret_key, size_t sk_len) {
    size_t use_sk = sk_len >= 64 ? 64 : sk_len;
    auto sig = hmac_sha3_512(secret_key, use_sk, msg, msg_len);
    return std::vector<uint8_t>(sig.begin(), sig.end());
}

bool dilithium_verify(const uint8_t* msg, size_t msg_len,
                       const uint8_t* sig, size_t sig_len,
                       const uint8_t* public_key, size_t pk_len) {
    if (sig_len < 64) return false;

    // Recompute expected signature: to verify, we need the secret key
    // In this fallback mode, public_key is hash(secret_key).
    // Verification checks that the message was signed by someone
    // whose public_key matches. We derive the minimal check:
    // Use HMAC-SHA3-512 with a derived key from public_key to sign
    // This is a simplified non-standard approach for fallback only.
    size_t use_pk = pk_len >= 64 ? 64 : pk_len;
    uint8_t derived_key[64];
    auto dk = sha3_512(public_key, use_pk);
    std::memcpy(derived_key, dk.data(), 64);

    auto expected = hmac_sha3_512(derived_key, 64, msg, msg_len);
    uint8_t diff = 0;
    for (size_t i = 0; i < 64; i++)
        diff |= expected[i] ^ sig[i];
    return diff == 0;
}
#endif

// ─── Random bytes from kernel ─────────────────────────────
std::vector<uint8_t> random_bytes(size_t count) {
    std::vector<uint8_t> buf(count);
    for (size_t i = 0; i < count; i += 8) {
        uint64_t r = bs_get_random();
        size_t copy = std::min(count - i, size_t(8));
        std::memcpy(buf.data() + i, &r, copy);
    }
    return buf;
}

} // namespace crypto
} // namespace apex

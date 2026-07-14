#ifndef APEX_CRYPTO_PRIMITIVES_H
#define APEX_CRYPTO_PRIMITIVES_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <string>

namespace apex {
namespace crypto {

// SHA3-512 (software implementation, no external deps)
std::array<uint8_t, 64> sha3_512(const uint8_t* data, size_t len);

// AES-256-GCM (software)
std::vector<uint8_t> aes256_gcm_encrypt(const uint8_t* plain, size_t len,
                                         const uint8_t* key, size_t key_len);
std::vector<uint8_t> aes256_gcm_decrypt(const uint8_t* cipher, size_t len,
                                         const uint8_t* key, size_t key_len);

// Dilithium wrapper (uses liboqs if available, otherwise stub)
struct DilithiumKeypair {
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> secret_key;
    bool valid = false;
};
DilithiumKeypair generate_dilithium_keypair();
std::vector<uint8_t> dilithium_sign(const uint8_t* msg, size_t msg_len,
                                     const uint8_t* secret_key, size_t sk_len);
bool dilithium_verify(const uint8_t* msg, size_t msg_len,
                       const uint8_t* sig, size_t sig_len,
                       const uint8_t* public_key, size_t pk_len);

// Utility
std::vector<uint8_t> random_bytes(size_t count);

} // namespace crypto
} // namespace apex

#endif

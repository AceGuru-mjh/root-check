#ifndef APEX_ROOT_OQS_SIGNATURE_H
#define APEX_ROOT_OQS_SIGNATURE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <mutex>

namespace apex {
namespace crypto {

class OqsSignature {
public:
    static OqsSignature& getInstance();

    bool generateKeyPair(std::vector<uint8_t>& publicKey, std::vector<uint8_t>& privateKey);
    std::vector<uint8_t> sign(const std::vector<uint8_t>& data, const std::vector<uint8_t>& privateKey);
    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::vector<uint8_t>& publicKey);

    // Key sizes
    size_t publicKeySize() const { return pubKeySize; }
    size_t secretKeySize() const { return secKeySize; }
    size_t signatureSize() const { return sigSize; }
    bool isAvailable() const { return available; }

    // Base64 (thread-safe, no OQS state needed)
    std::string base64Encode(const std::vector<uint8_t>& data, bool urlSafe = false);
    std::vector<uint8_t> base64Decode(const std::string& encoded, bool urlSafe = false);

    // String helpers
    static std::vector<uint8_t> stringToBytes(const std::string& str);
    static std::string bytesToString(const std::vector<uint8_t>& bytes);

    // SHA3-512 convenience
    static std::array<uint8_t, 64> hash512(const uint8_t* data, size_t len);

private:
    OqsSignature();
    ~OqsSignature();
    OqsSignature(const OqsSignature&) = delete;
    OqsSignature& operator=(const OqsSignature&) = delete;

    mutable std::mutex mtx;
    void* sig = nullptr;
    bool available = false;
    size_t pubKeySize = 0;
    size_t secKeySize = 0;
    size_t sigSize = 0;
};

} // namespace crypto
} // namespace apex

#endif // APEX_ROOT_OQS_SIGNATURE_H

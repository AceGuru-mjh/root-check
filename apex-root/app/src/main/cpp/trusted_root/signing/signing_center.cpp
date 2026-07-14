#include "signing_center.h"
#include "trusted_root/key_management/key_derivation.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include "trusted_root/crypto/oqs_signature.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <mutex>
#include <android/log.h>

namespace apex {
namespace signing {

static std::mutex g_mutex;
static std::vector<LayerResult> g_pending_layers;
static key::DeviceKeyPair g_keypair;
static bool g_initialized = false;

bool initialize_signing_center() {
    g_keypair = key::get_device_keypair();
    g_initialized = !g_keypair.public_key.empty();

    auto& oqs = crypto::OqsSignature::getInstance();
    if (g_initialized && oqs.isAvailable()) {
        __android_log_print(ANDROID_LOG_DEBUG, "APEX-SIGN",
            "Signing center ready with PQ crypto (pk=%zu, sk=%zu, sig=%zu)",
            oqs.publicKeySize(), oqs.secretKeySize(), oqs.signatureSize());
    }
    return g_initialized;
}

bool sign_layer_result(LayerResult& result) {
    if (!g_initialized) return false;

    auto hash = crypto::sha3_512(
        reinterpret_cast<const uint8_t*>(&result.data_hash),
        sizeof(result.data_hash));
    std::memcpy(result.data_hash, hash.data(), 64);

    auto& oqs = crypto::OqsSignature::getInstance();
    auto sig = oqs.sign(
        std::vector<uint8_t>(result.data_hash, result.data_hash + 64),
        g_keypair.secret_key);
    return !sig.empty();
}

bool verify_report(const uint8_t* report, size_t report_len,
                    const uint8_t* sig, size_t sig_len) {
    if (!g_initialized) return false;

    auto hash = crypto::sha3_512(report, report_len);
    auto& oqs = crypto::OqsSignature::getInstance();
    return oqs.verify(
        std::vector<uint8_t>(hash.data(), hash.data() + 64),
        std::vector<uint8_t>(sig, sig + sig_len),
        g_keypair.public_key);
}

void submit_layer(const LayerResult& layer) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pending_layers.push_back(layer);
}

SignedReport finalize_report() {
    std::lock_guard<std::mutex> lock(g_mutex);
    SignedReport report;
    report.version = 1;
    report.layers = g_pending_layers;

    float max_risk = 0;
    for (const auto& layer : report.layers) {
        if (!layer.success) max_risk = 1.0f;
    }
    report.overall_risk = max_risk;

    auto hash = crypto::sha3_512(
        reinterpret_cast<const uint8_t*>(report.layers.data()),
        report.layers.size() * sizeof(LayerResult));
    std::memcpy(report.report_hash, hash.data(), 64);

    auto& oqs = crypto::OqsSignature::getInstance();
    report.dilithium_signature = oqs.sign(
        std::vector<uint8_t>(report.report_hash, report.report_hash + 64),
        g_keypair.secret_key);

    auto daemon_hash = crypto::sha3_512(
        report.dilithium_signature.data(),
        report.dilithium_signature.size());
    std::memcpy(report.daemon_sig, daemon_hash.data(), 64);

    g_pending_layers.clear();

    persist_report(report);
    return report;
}

static SignedReport g_last_report;
static bool g_has_last_report = false;

std::optional<SignedReport> get_last_report() {
    if (!g_has_last_report) return std::nullopt;
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_last_report;
}

void persist_report(const SignedReport& report) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_last_report = report;
    g_has_last_report = true;

    // Also persist to file for durability across process restarts
    // Write to app's data directory via /data/local/tmp/apex_last_report
    // In production, this would use the app's internal storage
    auto fd = bs_openat(-100, "/data/local/tmp/apex_last_report.bin",
                        0x41, 0600); // O_CREAT|O_RDWR
    if (fd >= 0) {
        // Serialize report version + hash + signature
        uint8_t header[8];
        std::memcpy(header, "APEXRP", 6);
        uint32_t version = 1;
        std::memcpy(header + 6, &version, 2);
        bs_write(fd, header, 8);
        bs_write(fd, report.report_hash, 64);
        uint32_t sig_size = report.dilithium_signature.size();
        bs_write(fd, &sig_size, 4);
        if (sig_size > 0)
            bs_write(fd, report.dilithium_signature.data(), sig_size);
        bs_close(fd);
    }
}

bool signing_center::submit_result(const LayerResult& layer) {
    submit_layer(layer);
    return true;
}

SignedReport signing_center::finalize() {
    return finalize_report();
}

} // namespace signing
} // namespace apex

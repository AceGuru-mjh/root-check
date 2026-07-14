#ifndef APEX_ROOT_SIGNING_CENTER_H
#define APEX_ROOT_SIGNING_CENTER_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <optional>
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"

namespace apex {
namespace signing {

struct LayerResult {
    uint32_t service_id;
    bool success;
    float confidence;
    uint8_t data_hash[64];
    uint64_t timestamp_ns;
};

struct SignedReport {
    std::vector<LayerResult> layers;
    uint8_t report_hash[64];
    std::vector<uint8_t> dilithium_signature;
    uint8_t daemon_sig[64];
    float overall_risk;
    uint32_t version;
};

bool initialize_signing_center();
bool sign_layer_result(LayerResult& result);
bool verify_report(const uint8_t* report, size_t report_len,
                    const uint8_t* sig, size_t sig_len);
std::optional<SignedReport> get_last_report();
void submit_layer(const LayerResult& layer);
SignedReport finalize_report();
void persist_report(const SignedReport& report);

namespace signing_center {
    bool submit_result(const LayerResult& layer);
    SignedReport finalize();
}

} // namespace signing
} // namespace apex

#endif // APEX_ROOT_SIGNING_CENTER_H

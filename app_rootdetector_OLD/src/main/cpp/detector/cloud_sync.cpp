#include "cloud_sync.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

CloudSync &CloudSync::instance() {
    static CloudSync sync;
    return sync;
}

bool CloudSync::init(const std::string &base_path) {
    base_path_ = base_path;
    local_version_ = 0;
    initialized_ = false;
    return loadLocalRuleIndex();
}

bool CloudSync::loadLocalRuleIndex() {
    std::string index_path = base_path_ + "/.rule_index";
    std::ifstream file(index_path, std::ios::binary);
    if (!file.is_open()) {
        local_version_ = 0;
        return true;
    }

    file.read(reinterpret_cast<char *>(&local_version_), sizeof(local_version_));
    if (!file) local_version_ = 0;

    file.seekg(0, std::ios::end);
    size_t remaining = (size_t)file.tellg() - sizeof(local_version_);
    file.seekg(sizeof(local_version_));

    local_rule_index_.resize(remaining);
    file.read(reinterpret_cast<char *>(local_rule_index_.data()), remaining);

    initialized_ = true;
    return true;
}

SyncResult CloudSync::checkForUpdates() {
    SyncResult result;
    result.success = false;
    result.new_rules_added = 0;
    result.rules_updated = 0;
    result.rules_removed = 0;
    result.current_version = local_version_;

    std::string url = "https://rules.rootguard.app/v1/diff?from=" +
                      std::to_string(local_version_);

    std::vector<uint8_t> response;
    if (!fetchFromUrl(url, response)) {
        result.error_msg = "Network unavailable (offline-first ok)";
        result.success = true;
        return result;
    }

    if (response.size() < sizeof(DiffRulePackage::version)) {
        result.error_msg = "Invalid response";
        return result;
    }

    DiffRulePackage pkg;
    memcpy(&pkg.version, response.data(), sizeof(pkg.version));

    if (pkg.version <= local_version_) {
        result.success = true;
        return result;
    }

    if (response.size() >= sizeof(uint32_t) * 4 + 64 + 32) {
        size_t off = 0;
        memcpy(&pkg.version, response.data() + off, sizeof(pkg.version)); off += 4;
        memcpy(&pkg.base_version, response.data() + off, sizeof(pkg.base_version)); off += 4;
        memcpy(&pkg.rule_count, response.data() + off, sizeof(pkg.rule_count)); off += 4;
        memcpy(&pkg.total_size, response.data() + off, sizeof(pkg.total_size)); off += 4;
        memcpy(pkg.signature, response.data() + off, 64); off += 64;
        memcpy(pkg.hash, response.data() + off, 32); off += 32;

        size_t payload_size = response.size() - off;
        pkg.encrypted_payload.resize(payload_size);
        memcpy(pkg.encrypted_payload.data(), response.data() + off, payload_size);

        if (verifyPackageSignature(pkg)) {
            result.success = true;
            result.current_version = pkg.version;
            result.new_rules_added = (int)pkg.rule_count;
        } else {
            result.error_msg = "Signature verification failed";
        }
    }

    return result;
}

bool CloudSync::downloadUpdate(SyncResult &result) {
    std::string url = "https://rules.rootguard.app/v1/pull/" +
                      std::to_string(result.current_version);
    std::vector<uint8_t> data;
    if (!fetchFromUrl(url, data)) return false;

    std::string path = base_path_ + "/.rule_update";
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    return true;
}

bool CloudSync::applyUpdate(const SyncResult &result) {
    if (!result.success) return false;

    local_version_ = result.current_version;

    std::string index_path = base_path_ + "/.rule_index";
    std::ofstream file(index_path, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char *>(&local_version_),
               sizeof(local_version_));
    file.write(reinterpret_cast<const char *>(local_rule_index_.data()),
               local_rule_index_.size());

    return true;
}

bool CloudSync::verifyPackageSignature(const DiffRulePackage &pkg) {
    uint8_t computed_hash[32];
    std::vector<uint8_t> data;
    data.resize(sizeof(pkg.version) + sizeof(pkg.base_version) +
                sizeof(pkg.rule_count) + sizeof(pkg.total_size) +
                pkg.encrypted_payload.size());

    size_t off = 0;
    memcpy(data.data() + off, &pkg.version, sizeof(pkg.version)); off += 4;
    memcpy(data.data() + off, &pkg.base_version, sizeof(pkg.base_version)); off += 4;
    memcpy(data.data() + off, &pkg.rule_count, sizeof(pkg.rule_count)); off += 4;
    memcpy(data.data() + off, &pkg.total_size, sizeof(pkg.total_size)); off += 4;
    memcpy(data.data() + off, pkg.encrypted_payload.data(), pkg.encrypted_payload.size());

    sha3_256(data.data(), data.size(), computed_hash);
    return memcmp(computed_hash, pkg.hash, 32) == 0;
}

bool CloudSync::fetchFromUrl(const std::string &url, std::vector<uint8_t> &data) {
    (void)url;
    data.clear();
    return false;
}

bool CloudSync::decompressDiff(const std::vector<uint8_t> &compressed,
                                std::vector<uint8_t> &decompressed) {
    decompressed = compressed;
    return true;
}

bool CloudSync::applyPatch(const std::vector<uint8_t> &diff,
                           const std::vector<uint8_t> &base,
                           std::vector<uint8_t> &output) {
    output = base;
    return true;
}

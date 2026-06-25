#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <string>
#include <vector>
#include <cstdint>

struct DiffRulePackage {
    uint32_t version;
    uint32_t base_version;
    uint32_t rule_count;
    uint32_t total_size;
    uint8_t signature[64];
    uint8_t hash[32];
    std::vector<uint8_t> encrypted_payload;
};

struct SyncResult {
    bool success;
    int new_rules_added;
    int rules_updated;
    int rules_removed;
    std::string error_msg;
    uint32_t current_version;
};

class CloudSync {
public:
    static CloudSync &instance();

    bool init(const std::string &base_path);
    SyncResult checkForUpdates();
    bool downloadUpdate(SyncResult &result);
    bool applyUpdate(const SyncResult &result);
    bool verifyPackageSignature(const DiffRulePackage &pkg);
    bool loadLocalRuleIndex();
    uint32_t getLocalVersion() const { return local_version_; }

    static const uint32_t CURRENT_RULE_VERSION = 1;
    static const size_t MAX_PACKAGE_SIZE = 512 * 1024;

private:
    CloudSync() = default;

    std::string base_path_;
    uint32_t local_version_;
    bool initialized_;
    std::vector<uint8_t> local_rule_index_;

    bool fetchFromUrl(const std::string &url, std::vector<uint8_t> &data);
    bool decompressDiff(const std::vector<uint8_t> &compressed,
                        std::vector<uint8_t> &decompressed);
    bool applyPatch(const std::vector<uint8_t> &diff,
                    const std::vector<uint8_t> &base,
                    std::vector<uint8_t> &output);
};

#endif

#ifndef APEX_ROOT_UPDATER_H
#define APEX_ROOT_UPDATER_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <functional>

namespace apex {
namespace updater {

struct RulePackage {
    uint32_t version;
    uint32_t rule_count;
    std::vector<uint8_t> data;
    std::vector<uint8_t> signature;
    uint64_t timestamp;
};

struct UpdateManifest {
    uint32_t latest_version;
    std::string download_url;
    std::string checksum;
    uint64_t package_size;
};

bool check_for_updates();
UpdateManifest fetch_manifest();
bool download_rules(UpdateManifest& manifest, RulePackage& pkg);
bool verify_package(const RulePackage& pkg, const uint8_t* public_key, size_t key_len);
bool apply_rules(const RulePackage& pkg);
bool rollback_rules(uint32_t to_version);

// Differential update support
struct DiffPackage {
    std::vector<uint8_t> added;
    std::vector<uint8_t> removed;
    uint32_t base_version;
    uint32_t target_version;
};

DiffPackage create_diff(const RulePackage& old_pkg, const RulePackage& new_pkg);
bool apply_diff(const DiffPackage& diff);

} // namespace updater
} // namespace apex

#endif // APEX_ROOT_UPDATER_H

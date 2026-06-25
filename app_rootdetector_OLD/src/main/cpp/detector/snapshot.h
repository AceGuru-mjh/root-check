#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <string>
#include <vector>
#include <cstdint>

struct EnvironmentSnapshot {
    uint64_t timestamp;
    std::string proc_maps;
    std::string mount_info;
    std::string process_list;
    std::string kernel_modules;
    std::string property_contexts;
    std::vector<uint8_t> encrypted_blob;
    uint8_t signature[64];
    bool captured;
};

class SnapshotManager {
public:
    static EnvironmentSnapshot capture();
    static bool saveToFile(const EnvironmentSnapshot &snap,
                           const std::string &path);
    static EnvironmentSnapshot loadFromFile(const std::string &path);
    static bool encryptSnapshot(EnvironmentSnapshot &snap,
                                const uint8_t *key);
    static bool verifySnapshotIntegrity(const EnvironmentSnapshot &snap);

private:
    static std::string readProcFile(const std::string &path);
    static std::string readDirEntries(const std::string &dir_path);
};

#endif

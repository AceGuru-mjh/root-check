#include "snapshot.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <dirent.h>
#include <unistd.h>

EnvironmentSnapshot SnapshotManager::capture() {
    EnvironmentSnapshot snap;
    snap.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    snap.captured = true;

    snap.proc_maps = readProcFile("/proc/self/maps");
    snap.mount_info = readProcFile("/proc/self/mountinfo");
    snap.process_list = readDirEntries("/proc");
    snap.kernel_modules = readProcFile("/proc/modules");
    snap.property_contexts = readProcFile("/proc/self/attr/current");

    return snap;
}

std::string SnapshotManager::readProcFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string SnapshotManager::readDirEntries(const std::string &dir_path) {
    std::string result;
    DIR *dir = opendir(dir_path.c_str());
    if (!dir) return "";

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        result += entry->d_name;
        result += "\n";
    }
    closedir(dir);
    return result;
}

bool SnapshotManager::saveToFile(const EnvironmentSnapshot &snap,
                                  const std::string &path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char *>(&snap.timestamp),
               sizeof(snap.timestamp));
    file.write(snap.signature, sizeof(snap.signature));

    auto write_str = [&file](const std::string &s) {
        uint32_t len = (uint32_t)s.length();
        file.write(reinterpret_cast<const char *>(&len), sizeof(len));
        file.write(s.data(), len);
    };

    write_str(snap.proc_maps);
    write_str(snap.mount_info);
    write_str(snap.process_list);
    write_str(snap.kernel_modules);
    write_str(snap.property_contexts);

    return file.good();
}

EnvironmentSnapshot SnapshotManager::loadFromFile(const std::string &path) {
    EnvironmentSnapshot snap;
    snap.captured = false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return snap;

    file.read(reinterpret_cast<char *>(&snap.timestamp), sizeof(snap.timestamp));
    file.read(reinterpret_cast<char *>(snap.signature), sizeof(snap.signature));

    auto read_str = [&file](std::string &s) {
        uint32_t len = 0;
        file.read(reinterpret_cast<char *>(&len), sizeof(len));
        s.resize(len);
        file.read(&s[0], len);
    };

    read_str(snap.proc_maps);
    read_str(snap.mount_info);
    read_str(snap.process_list);
    read_str(snap.kernel_modules);
    read_str(snap.property_contexts);

    snap.captured = file.good();
    return snap;
}

bool SnapshotManager::encryptSnapshot(EnvironmentSnapshot &snap,
                                       const uint8_t *key) {
    std::string combined = snap.proc_maps + snap.mount_info +
                           snap.process_list + snap.kernel_modules;

    snap.encrypted_blob.resize(combined.size());
    for (size_t i = 0; i < combined.size(); i++) {
        snap.encrypted_blob[i] = combined[i] ^ key[i % 32];
    }

    return true;
}

bool SnapshotManager::verifySnapshotIntegrity(const EnvironmentSnapshot &snap) {
    if (snap.proc_maps.empty() && snap.mount_info.empty()) return false;
    if (snap.timestamp == 0) return false;
    return snap.captured;
}

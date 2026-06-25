#include "hardware_id.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <openssl/sha.h>

HardwareIdentity HardwareIdExtractor::extract() {
    HardwareIdentity id;
    id.cpu_serial = readCpuSerial();
    id.tee_device_id = readTeeDeviceId();
    id.board_serial = readBoardSerial();
    id.fingerprint = readProperty("ro.build.fingerprint");
    id.has_tee = !id.tee_device_id.empty();

    std::string combined = id.cpu_serial + id.board_serial +
                           id.fingerprint + id.tee_device_id;
    id.device_id = hashIdentity(combined);
    id.has_unique_id = !combined.empty();

    id.device_key = deriveDeviceKey(id);
    return id;
}

std::string HardwareIdExtractor::readCpuSerial() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) return "";
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("Serial") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string serial = line.substr(pos + 1);
                serial.erase(0, serial.find_first_not_of(' '));
                return serial;
            }
        }
    }
    return "";
}

std::string HardwareIdExtractor::readTeeDeviceId() {
    std::vector<std::string> tee_paths = {
        "/sys/kernel/security/tomoyo/version",
        "/sys/firmware/devicetree/base/serial-number",
        "/proc/tee"
    };
    for (const auto &path : tee_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string content;
            std::getline(file, content);
            if (!content.empty()) return content;
        }
    }
    return "";
}

std::string HardwareIdExtractor::readBoardSerial() {
    std::ifstream file("/sys/class/dmi/id/board_serial");
    if (!file.is_open()) {
        file.open("/sys/firmware/devicetree/base/serial-number");
    }
    if (file.is_open()) {
        std::string serial;
        std::getline(file, serial);
        return serial;
    }
    return "";
}

std::string HardwareIdExtractor::readProperty(const std::string &key) {
    std::ifstream build_prop("/system/build.prop");
    if (!build_prop.is_open()) return "";
    std::string line;
    while (std::getline(build_prop, line)) {
        if (line.find(key) != std::string::npos) {
            auto pos = line.find('=');
            if (pos != std::string::npos) return line.substr(pos + 1);
        }
    }
    return "";
}

std::string HardwareIdExtractor::hashIdentity(const std::string &input) {
    if (input.empty()) return "";
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()),
           input.length(), hash);
    std::string result;
    const char *hex = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        result += hex[(hash[i] >> 4) & 0xf];
        result += hex[hash[i] & 0xf];
    }
    return result;
}

std::vector<uint8_t> HardwareIdExtractor::deriveDeviceKey(
    const HardwareIdentity &id) {
    std::string seed = id.cpu_serial + id.board_serial + id.fingerprint;
    if (seed.length() < 32) {
        seed += std::string(32 - seed.length(), '0');
    }
    seed = seed.substr(0, 32);
    return std::vector<uint8_t>(seed.begin(), seed.end());
}

bool HardwareIdExtractor::verifyDeviceBinding(const std::vector<uint8_t> &key) {
    auto current = extract();
    if (current.device_key.size() != key.size()) return false;
    return memcmp(current.device_key.data(), key.data(), key.size()) == 0;
}

#ifndef HARDWARE_ID_H
#define HARDWARE_ID_H

#include <string>
#include <vector>
#include <cstdint>

struct HardwareIdentity {
    std::string device_id;
    std::string cpu_serial;
    std::string tee_device_id;
    std::string board_serial;
    std::string mac_hash;
    std::string android_id;
    std::string fingerprint;
    std::vector<uint8_t> device_key;
    bool has_tee;
    bool has_unique_id;
};

class HardwareIdExtractor {
public:
    static HardwareIdentity extract();
    static std::vector<uint8_t> deriveDeviceKey(const HardwareIdentity &id);
    static bool verifyDeviceBinding(const std::vector<uint8_t> &key);

private:
    static std::string readCpuSerial();
    static std::string readTeeDeviceId();
    static std::string readBoardSerial();
    static std::string readProperty(const std::string &key);
    static std::string hashIdentity(const std::string &input);
};

#endif

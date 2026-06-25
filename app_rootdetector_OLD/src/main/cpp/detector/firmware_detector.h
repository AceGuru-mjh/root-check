#ifndef ROOTDETECTOR_FIRMWARE_DETECTOR_H
#define ROOTDETECTOR_FIRMWARE_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct FirmwareDetectDetail {
    /* Boot */
    std::string boot_hash;
    bool boot_magisk_gap;
    bool boot_magisk_ramdisk;
    bool boot_apatch_kernel;
    bool boot_kernelsu;
    std::vector<std::string> boot_rootkits;
    int boot_risk;

    /* Vendor */
    std::string vendor_hash;
    bool vendor_rootkit_so;
    std::vector<std::string> vendor_rootkit_sos;
    bool vendor_dlkm_rootkit;
    int vendor_risk;

    /* Recovery */
    std::string recovery_hash;
    bool recovery_custom;
    std::string recovery_type;
    int recovery_risk;

    /* TEE */
    bool tee_devices_found;
    std::vector<std::string> tee_keys;
    std::vector<std::string> sec_partition_hashes;
    int tee_risk;

    /* Logical partitions */
    std::string super_hash;
    bool vbmeta_verify_off;
    std::string dtbo_hash;
    bool persist_modified;
    int logical_risk;

    /* Kernel logs */
    bool has_pstore;
    std::vector<std::string> log_markers;
    int log_risk;

    /* Self-check */
    bool traced;
    bool hooked;
    int self_risk;

    std::vector<std::string> findings;
    bool detected;
};

class FirmwareDetector {
public:
    static DetectionResult detect();
    static FirmwareDetectDetail detectDetailed();

private:
    static std::string hashFile(const std::string& path, size_t max_read);
    static std::string hashBlockDevice(const std::string& path, size_t max_read);
    static std::string hashBlockDeviceStream(const std::string& path, size_t max_read);
    static std::string bytesToHex(const uint8_t* data, size_t len);
    static bool readBlockDevice(const std::string& path, uint8_t* buf, size_t offset, size_t size);
    static std::string findPartition(const std::vector<std::string>& candidates);
    static bool scanBootForGap(const std::string& path, int threshold);
    static void scanBootPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanVendorPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanRecoveryPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanSecureStorage(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanLogicalPartitions(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanKernelLogs(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
    static void scanSelfCheck(FirmwareDetectDetail& detail, std::vector<std::string>& findings);
};

#endif

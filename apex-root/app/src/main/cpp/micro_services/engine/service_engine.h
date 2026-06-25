#ifndef APEX_ROOT_SERVICE_ENGINE_H
#define APEX_ROOT_SERVICE_ENGINE_H

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace apex {
namespace engine {

enum class ScanLevel {
    QUICK = 0,
    STANDARD = 1,
    DEEP = 2,
    FORENSIC = 3
};

struct ScanConfig {
    ScanLevel level;
    bool cpu_bind;
    bool shuffle_services;
    std::vector<uint32_t> enabled_services;
    uint64_t noise_ns_min;
    uint64_t noise_ns_max;
};

struct ServiceResult {
    uint32_t service_id;
    std::string service_name;
    bool success;
    float confidence;
    uint64_t duration_ns;
    std::vector<uint8_t> output_data;
    std::string description;
};

struct ScanReport {
    std::vector<ServiceResult> results;
    uint64_t total_duration_ns;
    float overall_risk;
    int services_executed;
    int services_failed;
};

// Service interface
struct ServicePlugin {
    uint32_t id;
    const char* name;
    const char* version;
    bool (*init)();
    ServiceResult (*execute)(const ScanConfig& config);
    void (*cleanup)();
};

namespace service_engine {

__attribute__((visibility("default"))) bool initialize();
__attribute__((visibility("default"))) bool register_service(const ServicePlugin& plugin);
__attribute__((visibility("default"))) bool unregister_service(uint32_t service_id);
__attribute__((visibility("default"))) std::optional<ServicePlugin> get_service(uint32_t service_id);
__attribute__((visibility("default"))) ScanReport execute_scan(const ScanConfig& config);
__attribute__((visibility("default"))) void shutdown();

} // namespace service_engine
} // namespace engine
} // namespace apex

#endif // APEX_ROOT_SERVICE_ENGINE_H

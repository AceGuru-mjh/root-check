#ifndef APEX_ROOT_AI_ENGINE_H
#define APEX_ROOT_AI_ENGINE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>

namespace apex {
namespace ai {

struct AnomalyReport {
    float score;
    std::string description;
    uint32_t feature_id;
    uint64_t timestamp_ns;
};

struct BehaviorSample {
    std::vector<uint32_t> syscall_sequence;
    std::vector<uint64_t> timing_ns;
    std::vector<float> features;
    uint64_t timestamp_ns;
};

class AnomalyDetector {
public:
    bool initialize(const uint8_t* model_data, size_t model_size);
    AnomalyReport analyze(const BehaviorSample& sample);
    float compute_anomaly_score(const std::vector<float>& features);
    bool update_model(const uint8_t* model_data, size_t model_size);
    void reset_baseline();

private:
    bool m_initialized = false;
    std::vector<float> m_baseline_mean;
    std::vector<float> m_baseline_std;
    float m_threshold = 3.0f; // 3-sigma
};

class RuleGenerator {
public:
    std::string generate_rule(const AnomalyReport& report);
    bool validate_rule(const std::string& rule_yaml);
    std::string serialize_rule(const std::string& name, const std::string& pattern);
};

} // namespace ai
} // namespace apex

#endif // APEX_ROOT_AI_ENGINE_H

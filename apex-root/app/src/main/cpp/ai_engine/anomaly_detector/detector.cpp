#include "ai_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cmath>
#include <cstring>
#include <sstream>

namespace apex {
namespace ai {

bool AnomalyDetector::initialize(const uint8_t* model_data, size_t model_size) {
    if (!model_data || model_size < 8) return false;

    // Parse model header: 4 bytes feature_count, 4 bytes threshold
    uint32_t feature_count = 0;
    std::memcpy(&feature_count, model_data, 4);
    float threshold = 0;
    std::memcpy(&threshold, model_data + 4, 4);

    size_t expected = 8 + feature_count * 8; // mean + std for each feature
    if (model_size < expected) return false;

    m_baseline_mean.resize(feature_count);
    m_baseline_std.resize(feature_count);

    for (uint32_t i = 0; i < feature_count; i++) {
        std::memcpy(&m_baseline_mean[i], model_data + 8 + i * 8, 4);
        std::memcpy(&m_baseline_std[i], model_data + 8 + i * 8 + 4, 4);
    }

    m_threshold = threshold;
    m_initialized = true;
    return true;
}

AnomalyReport AnomalyDetector::analyze(const BehaviorSample& sample) {
    AnomalyReport report;
    report.score = compute_anomaly_score(sample.features);
    report.timestamp_ns = bs_clock_ns();

    if (report.score > m_threshold) {
        report.description = "Anomaly detected (score: " + std::to_string(report.score) + ")";
    } else {
        report.description = "Behavior normal";
    }

    return report;
}

float AnomalyDetector::compute_anomaly_score(const std::vector<float>& features) {
    if (!m_initialized || features.size() != m_baseline_mean.size()) {
        return 0.0f;
    }

    float max_z = 0.0f;
    for (size_t i = 0; i < features.size(); i++) {
        float stddev = m_baseline_std[i];
        if (stddev < 0.0001f) stddev = 0.0001f;
        float z = std::abs((features[i] - m_baseline_mean[i]) / stddev);
        if (z > max_z) max_z = z;
    }

    return max_z;
}

bool AnomalyDetector::update_model(const uint8_t* model_data, size_t model_size) {
    return initialize(model_data, model_size);
}

void AnomalyDetector::reset_baseline() {
    m_baseline_mean.clear();
    m_baseline_std.clear();
    m_initialized = false;
}

std::string RuleGenerator::generate_rule(const AnomalyReport& report) {
    std::ostringstream oss;
    oss << "rule_anomaly_" << report.timestamp_ns << ":\n";
    oss << "  description: \"" << report.description << "\"\n";
    oss << "  confidence: " << report.score << "\n";
    oss << "  action: alert\n";
    return oss.str();
}

bool RuleGenerator::validate_rule(const std::string& rule_yaml) {
    // Basic YAML structure validation
    return rule_yaml.find("rule_") == 0 && rule_yaml.find("description:") != std::string::npos;
}

std::string RuleGenerator::serialize_rule(const std::string& name, const std::string& pattern) {
    std::ostringstream oss;
    oss << "rule_" << name << ":\n";
    oss << "  pattern: \"" << pattern << "\"\n";
    oss << "  version: 1\n";
    return oss.str();
}

} // namespace ai
} // namespace apex

#ifndef APEX_ROOT_YAML_PARSER_H
#define APEX_ROOT_YAML_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace apex {
namespace engine {

struct WorkflowStep {
    uint32_t service_id;
    std::string service_name;
    std::map<std::string, std::string> parameters;
    int timeout_ms;
    bool optional;
};

struct Workflow {
    std::string name;
    std::string description;
    uint32_t version;
    std::vector<WorkflowStep> steps;
    std::vector<uint32_t> enabled_services;
    bool shuffle;
    bool cpu_bind;
};

class YamlParser {
public:
    std::optional<Workflow> parse(const std::string& yaml_content);
    std::vector<Workflow> parse_all(const std::string& yaml_content);
    std::string serialize(const Workflow& workflow);

private:
    std::string trim(const std::string& str);
    std::vector<std::string> split_lines(const std::string& content);
    std::string get_value(const std::string& line, const std::string& key);
};

} // namespace engine
} // namespace apex

#endif // APEX_ROOT_YAML_PARSER_H

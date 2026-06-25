#include "yaml_parser.h"
#include <sstream>
#include <algorithm>

namespace apex {
namespace engine {

std::string YamlParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

std::vector<std::string> YamlParser::split_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string YamlParser::get_value(const std::string& line, const std::string& key) {
    std::string search = key + ":";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return "";
    std::string val = line.substr(pos + search.length());
    val = trim(val);
    // Remove quotes
    if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
        val = val.substr(1, val.size() - 2);
    }
    return val;
}

std::optional<Workflow> YamlParser::parse(const std::string& yaml_content) {
    Workflow wf;
    auto lines = split_lines(yaml_content);
    if (lines.empty()) return std::nullopt;

    // Parse top-level fields
    // Simple YAML parser for our specific format
    WorkflowStep current_step;
    bool in_step = false;
    bool in_parameters = false;
    size_t param_indent = 0;

    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        size_t indent = line.find_first_not_of(" \t");
        if (indent == std::string::npos) indent = 0;

        if (in_parameters) {
            if (indent > param_indent && trimmed.find(':') != std::string::npos) {
                size_t colon = trimmed.find(':');
                std::string key = trim(trimmed.substr(0, colon));
                std::string val = trim(trimmed.substr(colon + 1));
                if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                    val = val.substr(1, val.size() - 2);
                }
                current_step.parameters[key] = val;
                continue;
            } else {
                in_parameters = false;
            }
        }

        if (trimmed.find("name:") == 0) {
            wf.name = get_value(trimmed, "name");
        } else if (trimmed.find("description:") == 0) {
            wf.description = get_value(trimmed, "description");
        } else if (trimmed.find("version:") == 0) {
            wf.version = std::stoul(get_value(trimmed, "version"));
        } else if (trimmed.find("shuffle:") == 0) {
            wf.shuffle = get_value(trimmed, "shuffle") == "true";
        } else if (trimmed.find("cpu_bind:") == 0) {
            wf.cpu_bind = get_value(trimmed, "cpu_bind") == "true";
        } else if (trimmed.find("- service_id:") == 0 || trimmed.find("service_id:") == 0) {
            if (in_step) {
                wf.steps.push_back(current_step);
            }
            current_step = WorkflowStep();
            current_step.service_id = std::stoul(get_value(trimmed, "service_id"));
            in_step = true;
        } else if (trimmed.find("service_name:") == 0) {
            current_step.service_name = get_value(trimmed, "service_name");
        } else if (trimmed.find("timeout_ms:") == 0) {
            current_step.timeout_ms = std::stoi(get_value(trimmed, "timeout_ms"));
        } else if (trimmed.find("optional:") == 0) {
            current_step.optional = get_value(trimmed, "optional") == "true";
        } else if (trimmed.find("parameters:") == 0 || trimmed == "parameters:") {
            in_parameters = true;
            param_indent = indent;
        }
    }

    // Push last step
    if (in_step) {
        wf.steps.push_back(current_step);
    }

    return wf;
}

std::vector<Workflow> YamlParser::parse_all(const std::string& yaml_content) {
    std::vector<Workflow> workflows;
    auto lines = split_lines(yaml_content);

    std::string current_yaml;
    bool in_workflow = false;

    for (const auto& line : lines) {
        if (line.find("workflow:") == 0 || line.find("- workflow:") == 0) {
            if (in_workflow && !current_yaml.empty()) {
                auto wf = parse(current_yaml);
                if (wf) workflows.push_back(*wf);
            }
            current_yaml = line + "\n";
            in_workflow = true;
        } else if (in_workflow) {
            current_yaml += line + "\n";
        }
    }

    if (in_workflow && !current_yaml.empty()) {
        auto wf = parse(current_yaml);
        if (wf) workflows.push_back(*wf);
    }

    return workflows;
}

std::string YamlParser::serialize(const Workflow& workflow) {
    std::ostringstream oss;
    oss << "name: \"" << workflow.name << "\"\n";
    oss << "description: \"" << workflow.description << "\"\n";
    oss << "version: " << workflow.version << "\n";
    oss << "shuffle: " << (workflow.shuffle ? "true" : "false") << "\n";
    oss << "cpu_bind: " << (workflow.cpu_bind ? "true" : "false") << "\n";
    oss << "steps:\n";
    for (const auto& step : workflow.steps) {
        oss << "  - service_id: " << step.service_id << "\n";
        oss << "    service_name: \"" << step.service_name << "\"\n";
        oss << "    timeout_ms: " << step.timeout_ms << "\n";
        oss << "    optional: " << (step.optional ? "true" : "false") << "\n";
    }
    return oss.str();
}

} // namespace engine
} // namespace apex

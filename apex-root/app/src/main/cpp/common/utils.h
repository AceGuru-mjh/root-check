#ifndef APEX_ROOT_UTILS_H
#define APEX_ROOT_UTILS_H

#include <cstdint>
#include <cstddef>
#include <string>

namespace apex {
namespace utils {

bool file_exists(const char* path);
ssize_t read_file(const char* path, char* buf, size_t size);
bool write_file(const char* path, const char* content, size_t len);
bool delete_path(const char* path);
bool exec_su_command(const char* cmd);
bool exec_su_command_quiet(const char* cmd);
std::string sha256_hash(const uint8_t* data, size_t len);

} // namespace utils
} // namespace apex

#endif

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

// v1.1.3 P2-S1: 严格白名单校验 — 只允许 [a-zA-Z0-9._-]
// 用于校验即将拼入 shell 命令的变量 (serialno / sandbox_name / file path 等),
// 防止 '; $ ` ( ) < > | & 空格' 等 shell 元字符造成命令注入。
// 调用方应在 snprintf 拼接前调用本函数, 不通过则拒绝执行。
bool is_safe_value(const char* s);

// v1.1.3 P2-S1: 路径白名单校验 — 允许 [a-zA-Z0-9._/-]
// 比 is_safe_value 多允许 '/' (路径分隔符), 但仍拒绝所有 shell 元字符
// ('; $ ` ( ) < > | & 空格 ' " * ? [ ] { } ~ 等)。
// 用于校验拼入 shell 命令的文件路径 (如 dd/mv/mount 的参数)。
bool is_safe_path(const char* s);

// ─── FIX-P2-KT P2-C5 (v1.1.3): 公共文件读取辅助函数 ───────
//
// 背景: 20 个 plugin.cpp 大量重复 `bs_openat + bs_read + bs_close + strstr`
// 模式 (审计 P2-C5 指出)。这些重复代码各有细微差异 (buffer 大小、read 次数、
// 错误处理), 易出 bug — 例如 layer22_emulator.cpp 的 P2-D13 EISDIR 漏报、
// ms003_mem_scanner.cpp:39 的 P1-C6 栈溢出都源自这类手写循环。
//
// 现提供两个封装好的辅助函数, 供未来 plugin 重构时替换:
//   - read_file_to_string: 读全文到 std::string, 自动循环 read 到 EOF
//   - read_file_to_buffer: 读到固定 buffer (NUL 终止), 适合小文件
//
// 二者均使用 bs_openat/bs_read/bs_close (绕过 libc syscall hook),
// 且自动循环读取 (修复旧 read_file 单次 read 截断 4KB 的 bug)。
// 本次只新增 API, 不重写已存在的 plugin (避免大范围改动), 后续 v1.2.0
// 统一重构 plugin 时再迁移调用方。

// 读取文件内容到 std::string (循环 read 直到 EOF)。
// 成功返回 true, 失败 (文件不存在/权限不足/路径是目录等) 返回 false。
// max_len 限制最大读取量防恶意大文件耗尽内存 (默认 4MB)。
// 注: 若文件 > max_len, 截断到 max_len 后返回 true (部分内容)。
bool read_file_to_string(const char* path, std::string& out,
                         size_t max_len = 4 * 1024 * 1024);

// 读取文件内容到 buffer (最多 buf_size-1 字节, NUL 终止)。
// 循环 read 直到 EOF 或 buffer 满, 修复旧 read_file 单次 read 截断 bug。
// 返回实际读取的字节数 (不含 NUL); 失败 (open 失败/read 全失败) 返回 -1。
// 注: 与旧 read_file 共存以维持向后兼容, 新代码应优先用本函数。
int64_t read_file_to_buffer(const char* path, char* buf, size_t buf_size);

} // namespace utils
} // namespace apex

#endif

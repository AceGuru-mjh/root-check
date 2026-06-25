#ifndef APEX_ROOT_SELF_PROTECTION_H
#define APEX_ROOT_SELF_PROTECTION_H

#include <cstdint>
#include <string>

namespace apex {
namespace protection {

void init_protection();
bool check_debugger();
bool check_injection();
bool check_code_integrity();
bool check_hook();
bool check_breakpoint();
bool verify_integrity();

} // namespace protection
} // namespace apex

#endif
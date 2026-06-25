#ifndef APEX_ROOT_SELINUX_CONTEXT_H
#define APEX_ROOT_SELINUX_CONTEXT_H

#include <cstddef>

bool detectSELinuxContextJump();
bool detectSELinuxPermissive();
bool detectSELinuxPolicyModification();
int  runSELinuxFullScan(char* out_report, size_t out_size);

#endif

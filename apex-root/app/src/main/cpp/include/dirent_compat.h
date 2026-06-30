#ifndef APEX_ROOT_DIRENT_COMPAT_H
#define APEX_ROOT_DIRENT_COMPAT_H

#include <cstdint>

#ifndef DT_DIR
#define DT_DIR 4
#endif
#ifndef DT_LNK
#define DT_LNK 10
#endif
#ifndef DT_REG
#define DT_REG 8
#endif
#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#endif

// apex_dirent64 is defined in bare_syscall/syscall_bridge.h.
// Include that header to use the struct.

#endif

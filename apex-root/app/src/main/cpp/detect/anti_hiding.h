#ifndef APEX_ROOT_ANTI_HIDING_H
#define APEX_ROOT_ANTI_HIDING_H

#include <cstddef>

// Shamiko detection
bool detectShamiko();
bool detectShamikoSELinuxTrick();
bool detectShamikoWhiteList();
int  shamikoFullScan(char* out_report, size_t out_size);

// ZygiskNext detection
bool detectZygiskNext();
bool detectZygiskNextMemfd();
bool detectZygiskNextIsolation();
int  zygiskNextFullScan(char* out_report, size_t out_size);

// Generic anti-hiding probes
bool detectProcessHiding();
bool detectMountNamespaceHiding();
bool detectSyscallTableHook();

#endif

#ifndef APEX_ROOT_LAYER3_MEM_H
#define APEX_ROOT_LAYER3_MEM_H

#include <cstddef>

bool detectSuspiciousMemory();
bool detectHiddenProcessMemory();

// Deep memory fingerprint scan
int  deepMemoryFingerprintScan(char* out_report, size_t out_size);
bool detectMagiskMemory();
bool detectZygiskMemory();
bool detectLSPosedMemory();
bool detectShamikoMemory();
bool detectFridaMemory();
int  countRWXPages();

enum MemFingerprintType {
    MEM_FINGERPRINT_NONE = 0,
    MEM_FINGERPRINT_MAGISK = 1,
    MEM_FINGERPRINT_ZYGISK = 2,
    MEM_FINGERPRINT_LSPOSED = 4,
    MEM_FINGERPRINT_SHAMIKO = 8,
    MEM_FINGERPRINT_FRIDA = 16,
    MEM_FINGERPRINT_XPOSED = 32,
    MEM_FINGERPRINT_SUBSTRATE = 64,
    MEM_FINGERPRINT_DOBBY = 128,
    MEM_FINGERPRINT_WHALE = 256,
    MEM_FINGERPRINT_ANON_EXEC = 512,
    MEM_FINGERPRINT_MEMFD = 1024,
    MEM_FINGERPRINT_ZYGISKNEXT = 2048,
};

// Full fingerprint scan returning bitmask
int fullMemoryFingerprintScan();

#endif

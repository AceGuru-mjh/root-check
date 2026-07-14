#ifndef APEX_ROOT_LAYER13_FIRMWARE_H
#define APEX_ROOT_LAYER13_FIRMWARE_H

#include <cstddef>

bool detectFirmwareTampering();
bool detectBootPartitionModified();
bool detectVendorPartitionModified();
bool detectAVBStatus();
bool detectRecoveryPartition();
int  firmwareFullScan(char* out_report, size_t out_size);

#endif

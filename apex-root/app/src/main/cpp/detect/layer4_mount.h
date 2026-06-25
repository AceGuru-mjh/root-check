#ifndef APEX_ROOT_LAYER4_MOUNT_H
#define APEX_ROOT_LAYER4_MOUNT_H

bool detectSuspiciousMounts();
bool detectMagiskMount();
bool detectKernelSUOverlay();
bool detectNamespaceIsolation();

#endif

#!/system/bin/sh

# APEX-Root post-fs-data
# Early mount namespace isolation

log -t APEX-Root "post-fs-data: starting early hide"

# Unshare mount namespace
if unshare -m sh -c 'mount -o bind /dev/null /data/adb/modules 2>/dev/null; echo "NS ready"' >/dev/null 2>&1; then
    log -t APEX-Root "Mount namespace isolated successfully"
    setprop apex.fw.ns ok
else
    log -t APEX-Root "Mount namespace isolation not supported"
    setprop apex.fw.ns fail
fi

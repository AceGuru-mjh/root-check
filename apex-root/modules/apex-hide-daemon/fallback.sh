#!/system/bin/sh
# apex-hide-daemon — fallback.sh
# 当守护进程二进制不可用时，使用纯 shell + mount 实现基础隐藏

APEX_HIDE_DIR=/data/adb/apex-hide
EMPTY_DIR="$APEX_HIDE_DIR/empty"
WHITELIST="$APEX_HIDE_DIR/conf/whitelist.conf"
HIDE_PATHS="$APEX_HIDE_DIR/conf/hide_paths.conf"

mkdir -p "$EMPTY_DIR"

echo "[fallback] Starting shell-based hide"

# 持续运行，监测目标应用启动并应用 mount namespace 隔离
# 这是个简化版本，真实实现应当用 inotifyd 或 poll /proc
while true; do
    # 对每个非白名单 UID 的进程，应用 mount namespace 隐藏
    for proc_dir in /proc/[0-9]*; do
        [ -d "$proc_dir" ] || continue
        PID=$(basename "$proc_dir")

        # 读取进程 UID
        UID_VAL=$(awk '/^Uid:/{print $2}' "$proc_dir/status" 2>/dev/null)
        [ -z "$UID_VAL" ] && continue

        # 检查是否在白名单
        WHITELISTED=0
        while IFS= read -r line; do
            line=$(echo "$line" | sed 's/#.*//; s/[[:space:]]//g')
            [ -z "$line" ] && continue
            if [ "$UID_VAL" = "$line" ]; then
                WHITELISTED=1
                break
            fi
        done < "$WHITELIST"

        [ "$WHITELISTED" = "1" ] && continue

        # 对非白名单进程，已经在各自 mount namespace 中
        # 这里只做全局 bind-mount 隐藏（对所有进程生效）
        break
    done

    # 全局 bind-mount 隐藏路径（对所有进程生效，简单粗暴）
    while IFS= read -r path; do
        path=$(echo "$path" | sed 's/#.*//; s/[[:space:]]//g')
        [ -z "$path" ] && continue
        if [ -e "$path" ]; then
            # 检查是否已经 bind-mount 过
            if ! mountpoint -q "$path" 2>/dev/null; then
                mount -o bind "$EMPTY_DIR" "$path" 2>/dev/null && \
                    echo "[fallback] Hidden: $path"
            fi
        fi
    done < "$HIDE_PATHS"

    # 每 30 秒检查一次（新挂载的路径需要重新隐藏）
    sleep 30
done

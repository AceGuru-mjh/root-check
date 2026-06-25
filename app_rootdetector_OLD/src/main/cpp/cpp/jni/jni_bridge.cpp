#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../bare_syscall/bare_syscall.h"
#include "../ipc/ipc.h"
#include "../sandbox_microkernel/plugin_interface.h"
#include "../crypto/core/crypto_core.h"

#define LOG_TAG "RootGuardJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeStartDaemon(JNIEnv *env, jobject thiz) {
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("Failed to fork daemon");
        return JNI_FALSE;
    }
    if (pid == 0) {
        setsid();
        /* 守护进程二进制由 CMake 输出到 bin/ 目录 */
        execl("/data/local/tmp/bin/trusted_daemon", "trusted_daemon", NULL);
        _exit(1);
    }
    /* 等待守护进程就绪（重试连接，最长 3s）*/
    for (int i = 0; i < 30; i++) {
        int fd = ipc_client_connect(IPC_DEFAULT_SOCKET_PATH);
        if (fd >= 0) { close(fd); return JNI_TRUE; }
        usleep(100000);
    }
    return JNI_TRUE;
}

static void json_escape(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 6 < out_size; i++) {
        unsigned char c = (unsigned char)in[i];
        switch (c) {
            case '\\': out[j++] = '\\'; out[j++] = '\\'; break;
            case '"':  out[j++] = '\\'; out[j++] = '"';  break;
            case '\n': out[j++] = '\\'; out[j++] = 'n';  break;
            case '\r': out[j++] = '\\'; out[j++] = 'r';  break;
            case '\t': out[j++] = '\\'; out[j++] = 't';  break;
            default:
                if (c < 0x20) {
                    int n = snprintf(out + j, out_size - j, "\\u%04x", c);
                    if (n > 0) j += (size_t)n;
                } else {
                    out[j++] = c;
                }
                break;
        }
    }
    out[j] = '\0';
}

static jstring result_to_json(JNIEnv *env, plugin_result_t *r) {
    char detail_esc[DETAIL_BUF_SIZE * 2 + 1];
    json_escape(r->detail, detail_esc, sizeof(detail_esc));

    char json[8192];
    snprintf(json, sizeof(json),
             "{\"layer_id\":%u,\"detected\":%s,\"confidence\":%u,"
             "\"risk_score\":%u,\"cost_ns\":%llu,\"detail\":\"%s\"}",
             r->layer_id,
             r->detected ? "true" : "false",
             r->confidence, r->risk_score,
             (unsigned long long)r->cost_ns, detail_esc);
    return env->NewStringUTF(json);
}

JNIEXPORT jstring JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeRunDetection(JNIEnv *env, jobject thiz,
                                                               jint layer_mask) {
    ipc_session_t session;
    int sock_fd = ipc_client_connect(IPC_DEFAULT_SOCKET_PATH);
    if (sock_fd < 0)
        return env->NewStringUTF("{\"error\":\"Cannot connect to daemon\"}");

    if (ipc_handshake_client(sock_fd, &session) != 0) {
        close(sock_fd);
        return env->NewStringUTF("{\"error\":\"Handshake failed\"}");
    }

    plugin_result_t request;
    memset(&request, 0, sizeof(request));
    request.layer_id = (uint32_t)layer_mask;

    if (ipc_send_message(sock_fd, &session, IPC_MSG_DETECT_REQUEST,
                         (uint8_t *)&request, sizeof(request)) != 0) {
        close(sock_fd);
        return env->NewStringUTF("{\"error\":\"Send failed\"}");
    }

    ipc_message_t resp;
    memset(&resp, 0, sizeof(resp));
    if (ipc_recv_message(sock_fd, &session, &resp) != 0) {
        close(sock_fd);
        return env->NewStringUTF("{\"error\":\"Recv failed\"}");
    }
    close(sock_fd);

    plugin_result_t *result = (plugin_result_t *)resp.payload;
    return result_to_json(env, result);
}

JNIEXPORT jstring JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeQuickDetect(JNIEnv *env, jobject thiz) {
    ipc_session_t session;
    int sock_fd = ipc_client_connect(IPC_DEFAULT_SOCKET_PATH);
    if (sock_fd < 0)
        return env->NewStringUTF("{\"error\":\"daemon_not_running\"}");

    ipc_handshake_client(sock_fd, &session);

    plugin_result_t req;
    memset(&req, 0, sizeof(req));
    ipc_send_message(sock_fd, &session, IPC_MSG_DETECT_REQUEST,
                     (uint8_t *)&req, sizeof(req));

    ipc_message_t resp;
    memset(&resp, 0, sizeof(resp));
    ipc_recv_message(sock_fd, &session, &resp);
    close(sock_fd);

    plugin_result_t *r = (plugin_result_t *)resp.payload;
    return result_to_json(env, r);
}

} // extern "C"
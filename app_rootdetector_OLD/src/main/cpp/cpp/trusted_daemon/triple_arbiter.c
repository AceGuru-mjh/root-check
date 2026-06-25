#include "triple_arbiter.h"
#include "../bare_syscall/bare_syscall.h"
#include "../bare_syscall/cpu_control.h"
#include "../ipc/ipc.h"
#include "../crypto/core/crypto_core.h"
#include <string.h>

#define ARBITER_TIMEOUT_MS 30000
#define ARBITER_SOCKET_PATH "/data/local/tmp/.arbiter_"

static int g_arbiter_pids[3] = {-1, -1, -1};
static int g_arbiter_sockets[3] = {-1, -1, -1};
static int g_arbiter_initialized = 0;

static int hash_result(const arbiter_result_t *result, uint8_t *hash) {
    sha3_256((const uint8_t *)result, sizeof(arbiter_result_t), hash);
    return 0;
}

int triple_arbiter_init(void) {
    if (g_arbiter_initialized) return ARBITER_OK;

    for (int i = 0; i < 3; i++) {
        g_arbiter_pids[i] = -1;
        g_arbiter_sockets[i] = -1;
    }

    g_arbiter_initialized = 1;
    return ARBITER_OK;
}

static int fork_child(int instance_id, arbiter_result_t *result) {
    char sock_path[64];
    int pos = 0;
    const char *prefix = ARBITER_SOCKET_PATH;
    for (int i = 0; prefix[i] && pos < 63; i++) sock_path[pos++] = prefix[i];
    char id_str[4];
    int id_len = 0;
    int tmp = instance_id;
    do { id_str[id_len++] = '0' + (tmp % 10); tmp /= 10; } while (tmp > 0);
    for (int i = id_len - 1; i >= 0 && pos < 63; i--) sock_path[pos++] = id_str[i];
    sock_path[pos] = '\0';

    long pid = bare_fork();
    if (pid < 0) return ARBITER_ERR_FORK;

    if (pid == 0) {
        bind_to_cpu(instance_id % 4);

        int sock = ipc_client_connect(sock_path, 5000);
        if (sock < 0) bare_exit(1);

        ipc_message_t req;
        memset(&req, 0, sizeof(req));
        req.header.type = 0x01;
        req.header.seq = (uint32_t)instance_id;
        ipc_send_raw(sock, (const uint8_t *)&req, sizeof(req.header));

        ipc_message_t resp;
        memset(&resp, 0, sizeof(resp));
        int n = ipc_recv_raw(sock, (uint8_t *)&resp, sizeof(resp), 5000);
        if (n > 0) {
            result->result_id = resp.header.seq;
            result->root_detected = resp.payload[0];
            result->risk_score = (uint32_t)resp.payload[1];
        }

        bare_close(sock);
        bare_exit(0);
    }

    g_arbiter_pids[instance_id] = (int)pid;
    return ARBITER_OK;
}

int triple_arbiter_run_detection(arbiter_verdict_t *verdict) {
    if (!verdict) return ARBITER_ERR_IPC;

    memset(verdict, 0, sizeof(arbiter_verdict_t));

    for (int i = 0; i < 3; i++) {
        arbiter_result_t result;
        memset(&result, 0, sizeof(result));
        int ret = fork_child(i, &result);
        if (ret == ARBITER_OK) {
            verdict->instances[i].process_id = g_arbiter_pids[i];
            verdict->instances[i].result = result;
            verdict->instances[i].alive = 1;
        } else {
            verdict->instances[i].alive = 0;
        }
    }

    return ARBITER_OK;
}

int triple_arbiter_verify_consistency(const arbiter_result_t *a,
                                       const arbiter_result_t *b) {
    if (!a || !b) return 0;

    uint8_t hash_a[32], hash_b[32];
    hash_result(a, hash_a);
    hash_result(b, hash_b);

    int diff = 0;
    for (int i = 0; i < 32; i++) diff |= hash_a[i] ^ hash_b[i];

    if (diff != 0) return 0;

    if (a->root_detected != b->root_detected) return 0;

    int score_diff = a->risk_score > b->risk_score ?
                     a->risk_score - b->risk_score :
                     b->risk_score - a->risk_score;
    if (score_diff > 15) return 0;

    return 1;
}

int triple_arbiter_compare(arbiter_verdict_t *verdict) {
    if (!verdict) return ARBITER_ERR_IPC;

    int alive_count = 0;
    int match_12 = 0, match_13 = 0, match_23 = 0;

    for (int i = 0; i < 3; i++) {
        if (verdict->instances[i].alive) alive_count++;
    }

    if (alive_count < 2) {
        verdict->has_consensus = 0;
        verdict->mismatch_detected = 1;
        return ARBITER_ERR_MISMATCH;
    }

    match_12 = triple_arbiter_verify_consistency(
        &verdict->instances[0].result,
        &verdict->instances[1].result);

    match_13 = triple_arbiter_verify_consistency(
        &verdict->instances[0].result,
        &verdict->instances[2].result);

    match_23 = triple_arbiter_verify_consistency(
        &verdict->instances[1].result,
        &verdict->instances[2].result);

    int matches = match_12 + match_13 + match_23;

    if (matches >= 2) {
        verdict->has_consensus = 1;
        verdict->mismatch_detected = 0;

        if (match_12 || match_13) {
            verdict->consensus_root_detected =
                verdict->instances[0].result.root_detected;
            verdict->consensus_risk_score =
                verdict->instances[0].result.risk_score;
        } else {
            verdict->consensus_root_detected =
                verdict->instances[1].result.root_detected;
            verdict->consensus_risk_score =
                verdict->instances[1].result.risk_score;
        }
    } else {
        verdict->has_consensus = 0;
        verdict->mismatch_detected = 1;
        return ARBITER_ERR_MISMATCH;
    }

    verdict->verdict_ready = 1;
    return ARBITER_OK;
}

void triple_arbiter_report_mismatch(const arbiter_verdict_t *verdict) {
    for (int i = 0; i < 3; i++) {
        if (verdict->instances[i].alive) {
            bare_close(g_arbiter_sockets[i]);
        }
    }
}

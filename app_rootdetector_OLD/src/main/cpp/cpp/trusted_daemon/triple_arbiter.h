#ifndef TRIPLE_ARBITER_H
#define TRIPLE_ARBITER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBITER_OK              0
#define ARBITER_ERR_FORK        (-1)
#define ARBITER_ERR_IPC         (-2)
#define ARBITER_ERR_MISMATCH    (-3)
#define ARBITER_ERR_TIMEOUT     (-4)

typedef struct {
    uint32_t result_id;
    uint32_t layer_mask;
    int      root_detected;
    uint32_t risk_score;
    uint8_t  result_hash[32];
    uint32_t plugin_count;
    uint32_t anomalies_found;
} arbiter_result_t;

typedef struct {
    int process_id;
    arbiter_result_t result;
    int alive;
} arbiter_instance_t;

typedef struct {
    arbiter_instance_t instances[3];
    int consensus_root_detected;
    uint32_t consensus_risk_score;
    int has_consensus;
    int verdict_ready;
    int mismatch_detected;
} arbiter_verdict_t;

int triple_arbiter_init(void);
int triple_arbiter_run_detection(arbiter_verdict_t *verdict);
int triple_arbiter_compare(arbiter_verdict_t *verdict);
int triple_arbiter_verify_consistency(const arbiter_result_t *a,
                                       const arbiter_result_t *b);
void triple_arbiter_report_mismatch(const arbiter_verdict_t *verdict);

#ifdef __cplusplus
}
#endif

#endif

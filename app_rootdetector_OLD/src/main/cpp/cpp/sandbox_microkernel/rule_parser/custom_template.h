#ifndef CUSTOM_TEMPLATE_H
#define CUSTOM_TEMPLATE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEMPLATE_NAME_MAX 64
#define TEMPLATE_PATH_MAX 256
#define TEMPLATE_RULES_MAX 32
#define TEMPLATE_THRESHOLD_MIN 0
#define TEMPLATE_THRESHOLD_MAX 10000

typedef enum {
    TEMPLATE_RULE_PATH_EXISTS = 1,
    TEMPLATE_RULE_STRING_MATCH = 2,
    TEMPLATE_RULE_PROPERTY_MATCH = 3,
    TEMPLATE_RULE_SIDECHANNEL = 4,
    TEMPLATE_RULE_SOCKET_SCAN = 5,
    TEMPLATE_RULE_PROCESS_SCAN = 6
} template_rule_type_t;

typedef struct {
    template_rule_type_t rule_type;
    uint32_t enabled;
    uint32_t threshold_ns;
    char pattern[128];
    int severity;
} template_rule_t;

typedef struct {
    char name[TEMPLATE_NAME_MAX];
    uint32_t version;
    uint32_t rule_count;
    template_rule_t rules[TEMPLATE_RULES_MAX];
    uint8_t checksum[32];
} detection_template_t;

int template_init(void);
int template_load(const char *path, detection_template_t *tmpl);
int template_save(const char *path, const detection_template_t *tmpl);
int template_create(detection_template_t *tmpl, const char *name);
int template_add_rule(detection_template_t *tmpl, const template_rule_t *rule);
int template_remove_rule(detection_template_t *tmpl, uint32_t index);
int template_encrypt(const detection_template_t *tmpl, uint8_t *key);
int template_decrypt(detection_template_t *tmpl, const uint8_t *key);
int template_verify(const detection_template_t *tmpl);

#ifdef __cplusplus
}
#endif

#endif

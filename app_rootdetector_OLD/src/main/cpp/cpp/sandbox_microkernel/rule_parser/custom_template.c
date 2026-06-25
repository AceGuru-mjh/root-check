#include "custom_template.h"
#include "../../bare_syscall/bare_syscall.h"
#include "../../crypto/core/crypto_core.h"
#include <string.h>

#define TEMPLATE_DIR "/data/local/tmp/.rootguard_templates"
#define TEMPLATE_MAGIC 0x504D4554

static int g_template_initialized = 0;

int template_init(void) {
    long fd = bare_openat(-100, TEMPLATE_DIR, 0, 0);
    if (bare_is_error(fd)) {
        fd = bare_openat(-100, TEMPLATE_DIR, 0x41, 0700);
        if (bare_is_error(fd)) return -1;
    }
    bare_close((int)fd);
    g_template_initialized = 1;
    return 0;
}

int template_create(detection_template_t *tmpl, const char *name) {
    if (!tmpl || !name) return -1;
    memset(tmpl, 0, sizeof(detection_template_t));

    int i = 0;
    while (name[i] && i < TEMPLATE_NAME_MAX - 1) {
        tmpl->name[i] = name[i];
        i++;
    }
    tmpl->name[i] = '\0';
    tmpl->version = 1;
    tmpl->rule_count = 0;

    return 0;
}

int template_add_rule(detection_template_t *tmpl, const template_rule_t *rule) {
    if (!tmpl || !rule) return -1;
    if (tmpl->rule_count >= TEMPLATE_RULES_MAX) return -2;

    template_rule_t *dest = &tmpl->rules[tmpl->rule_count];
    dest->rule_type = rule->rule_type;
    dest->enabled = rule->enabled;
    dest->threshold_ns = rule->threshold_ns;
    dest->severity = rule->severity;

    int i = 0;
    while (rule->pattern[i] && i < 127) {
        dest->pattern[i] = rule->pattern[i];
        i++;
    }
    dest->pattern[i] = '\0';

    tmpl->rule_count++;
    return 0;
}

int template_remove_rule(detection_template_t *tmpl, uint32_t index) {
    if (!tmpl || index >= tmpl->rule_count) return -1;

    for (uint32_t i = index; i < tmpl->rule_count - 1; i++) {
        tmpl->rules[i] = tmpl->rules[i + 1];
    }
    tmpl->rule_count--;
    return 0;
}

int template_save(const char *path, const detection_template_t *tmpl) {
    if (!path || !tmpl) return -1;

    long fd = bare_openat(-100, path, 0x41, 0600);
    if (bare_is_error(fd)) return -2;

    uint32_t magic = TEMPLATE_MAGIC;
    bare_write((int)fd, (const void *)&magic, sizeof(magic));
    bare_write((int)fd, (const void *)tmpl, sizeof(detection_template_t));
    bare_close((int)fd);

    return 0;
}

int template_load(const char *path, detection_template_t *tmpl) {
    if (!path || !tmpl) return -1;

    long fd = bare_openat(-100, path, 0, 0);
    if (bare_is_error(fd)) return -2;

    uint32_t magic;
    bare_read((int)fd, &magic, sizeof(magic));
    if (magic != TEMPLATE_MAGIC) {
        bare_close((int)fd);
        return -3;
    }

    bare_read((int)fd, tmpl, sizeof(detection_template_t));
    bare_close((int)fd);

    return 0;
}

int template_encrypt(const detection_template_t *tmpl, uint8_t *key) {
    if (!tmpl || !key) return -1;

    uint8_t buffer[sizeof(detection_template_t)];
    memcpy(buffer, tmpl, sizeof(detection_template_t));

    for (size_t i = 0; i < sizeof(detection_template_t); i++) {
        buffer[i] ^= key[i % 32];
    }

    return 0;
}

int template_decrypt(detection_template_t *tmpl, const uint8_t *key) {
    if (!tmpl || !key) return -1;
    return template_encrypt(tmpl, (uint8_t *)key);
}

int template_verify(const detection_template_t *tmpl) {
    if (!tmpl) return 0;

    uint8_t hash[32];
    sha3_256((const uint8_t *)tmpl,
             sizeof(detection_template_t) - 32, hash);

    int match = 1;
    for (int i = 0; i < 32; i++) {
        if (hash[i] != tmpl->checksum[i]) {
            match = 0;
            break;
        }
    }

    return match;
}

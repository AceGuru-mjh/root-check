#ifndef APEX_ROOT_PLUGIN_INTERFACE_H
#define APEX_ROOT_PLUGIN_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define PLUGIN_API_VERSION 1
#define MAX_PLUGIN_NAME 64
#define MAX_PLUGIN_DESC 256

typedef enum {
    PLUGIN_STATUS_OK = 0,
    PLUGIN_STATUS_FAIL = -1,
    PLUGIN_STATUS_SKIP = 1
} plugin_status_t;

typedef struct {
    uint32_t version;
    char name[MAX_PLUGIN_NAME];
    char description[MAX_PLUGIN_DESC];
    uint32_t weight;
} plugin_info_t;

typedef plugin_status_t (*plugin_init_fn)(void);
typedef plugin_status_t (*plugin_exec_fn)(const char* config_json, char** result_json);
typedef void (*plugin_cleanup_fn)(void);

typedef struct {
    uint32_t api_version;
    plugin_info_t info;
    plugin_init_fn init;
    plugin_exec_fn execute;
    plugin_cleanup_fn cleanup;
} plugin_descriptor_t;

// Plugin must export this symbol - the engine finds it via dlsym
extern const plugin_descriptor_t apex_plugin_descriptor __attribute__((visibility("default")));

#ifdef __cplusplus
}
#endif

#endif

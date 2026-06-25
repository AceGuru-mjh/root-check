#ifndef HARDWARE_ID_H
#define HARDWARE_ID_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HWID_OK             0
#define HWID_ERR            (-1)
#define HWID_LEN            64

typedef struct {
    uint8_t  data[HWID_LEN];
    char     hex[128];
    uint32_t chip_id;
    uint64_t cpu_serial;
} hardware_id_t;

int hardware_id_get(hardware_id_t *hwid);
int hardware_id_get_serial(uint64_t *serial);
int hardware_id_get_chip_id(uint32_t *chip_id);
int hardware_id_derive_key(const hardware_id_t *hwid, uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif

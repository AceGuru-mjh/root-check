#ifndef SIGN_CENTER_H
#define SIGN_CENTER_H

#include <stdint.h>
#include <stddef.h>

/* 域C全局验签中心
 * 使用设备唯一密钥对所有检测结果进行 HMAC-SHA3-512 签名
 * 域A（UI）仅展示带合法签名的报告，无签名判定伪造
 */

void sign_center_init(const uint8_t *device_key, const uint8_t *device_id);

/* HMAC-SHA3-512 签名，输出64字节签名 */
int sign_center_sign(const uint8_t *data, size_t len,
                     uint8_t signature[64]);

/* 验签，返回 1=有效 0=无效 */
int sign_center_verify(const uint8_t *data, size_t len,
                       const uint8_t signature[64]);

#endif
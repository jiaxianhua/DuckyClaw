/**
 * @file tool_style_transfer.h
 * @brief Image style transfer tool header
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __TOOL_STYLE_TRANSFER_H__
#define __TOOL_STYLE_TRANSFER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Process image style transfer
 *
 * @param image_data JPEG image data
 * @param image_size Image size in bytes
 * @param style Style string (anime, cartoon, watercolor, sketch)
 * @param result_url Output buffer for result URL (min 512 bytes)
 * @param url_len Size of result_url buffer
 * @return OPERATE_RET OPRT_OK on success
 */
OPERATE_RET tool_style_transfer_process(uint8_t *image_data, uint32_t image_size,
                                        const char *style, char *result_url, size_t url_len);

/**
 * @brief Process style transfer and automatically display result
 *
 * @param image_data JPEG image data
 * @param image_size Image size in bytes
 * @param style Style string (anime, cartoon, watercolor, sketch)
 * @return OPERATE_RET OPRT_OK on success
 */
OPERATE_RET tool_style_transfer_and_display(uint8_t *image_data, uint32_t image_size,
                                            const char *style);

#ifdef __cplusplus
}
#endif

#endif /* __TOOL_STYLE_TRANSFER_H__ */

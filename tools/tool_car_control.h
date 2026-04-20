/**
 * @file tool_car_control.h
 * @brief MCP tool for controlling ESP32-based car via HTTP
 * @version 1.0
 * @date 2026-04-20
 *
 * @copyright Copyright (c) Tuya Inc. All Rights Reserved.
 */

#ifndef __TOOL_CAR_CONTROL_H__
#define __TOOL_CAR_CONTROL_H__

#include "tuya_cloud_types.h"

/**
 * @brief Register car control MCP tools
 *
 * Registers the following tools:
 *   - car_set_ip: Set the car's IP address
 *   - car_command: Send a single command to the car
 *   - car_sequence: Send a sequence of commands to the car
 *
 * @return OPERATE_RET OPRT_OK on success, error code on failure
 */
OPERATE_RET tool_car_control_register(void);

#endif /* __TOOL_CAR_CONTROL_H__ */

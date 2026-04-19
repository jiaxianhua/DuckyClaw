/**
 * @file tool_car_control.c
 * @brief MCP tool for controlling ESP32-based car via HTTP
 * @version 1.0
 * @date 2026-04-20
 *
 * @copyright Copyright (c) Tuya Inc. All Rights Reserved.
 */

#include "tool_car_control.h"
#include "tool_files.h"
#include "ai_mcp_server.h"
#include "http_client_interface.h"
#include "cJSON.h"
#include "tal_api.h"

#include <string.h>
#include <stdio.h>

/* Default car IP address */
static char s_car_ip[64] = "192.168.3.111";

/* HTTP timeout */
#define CAR_HTTP_TIMEOUT_MS 5000
#define CAR_RESP_BUF_SIZE   2048

/**
 * @brief Get string property from MCP property list
 */
static const char *__get_str_prop(const MCP_PROPERTY_LIST_T *props, const char *name)
{
    const MCP_PROPERTY_T *p = ai_mcp_property_list_find(props, name);
    if (p && p->type == MCP_PROPERTY_TYPE_STRING && p->default_val.str_val) {
        return p->default_val.str_val;
    }
    return NULL;
}

/**
 * @brief Send HTTP POST request to car
 */
static OPERATE_RET __car_http_post(const char *endpoint, const char *json_body,
                                   char *response_buf, size_t buf_size)
{
    if (!endpoint || !json_body || !response_buf || buf_size == 0) {
        return OPRT_INVALID_PARM;
    }

    PR_INFO("[car_control] ========== HTTP Request ==========");
    PR_INFO("[car_control] URL: http://%s:80%s", s_car_ip, endpoint);
    PR_INFO("[car_control] Method: POST");
    PR_INFO("[car_control] Body: %s", json_body);
    PR_INFO("[car_control] ====================================");

    /* Prepare request */
    http_client_header_t headers[1] = {
        {"Content-Type", "application/json"}
    };

    http_client_request_t request = {
        .host = s_car_ip,
        .port = 80,
        .path = endpoint,
        .cacert = NULL,
        .cacert_len = 0,
        .tls_no_verify = false,
        .method = "POST",
        .headers = headers,
        .headers_count = 1,
        .body = (const uint8_t *)json_body,
        .body_length = strlen(json_body),
        .timeout_ms = CAR_HTTP_TIMEOUT_MS
    };

    /* Allocate response buffer */
    uint8_t *resp_buf = (uint8_t *)claw_malloc(CAR_RESP_BUF_SIZE);
    if (!resp_buf) {
        PR_ERR("[car_control] Failed to allocate response buffer");
        return OPRT_MALLOC_FAILED;
    }

    http_client_response_t response = {
        .buffer = resp_buf,
        .buffer_length = CAR_RESP_BUF_SIZE
    };

    /* Send request */
    http_client_status_t status = http_client_request(&request, &response);

    OPERATE_RET rt = OPRT_OK;
    if (status != HTTP_CLIENT_SUCCESS) {
        PR_ERR("[car_control] ========== HTTP Response ==========");
        PR_ERR("[car_control] HTTP request failed, status=%d", status);
        PR_ERR("[car_control] Possible reasons:");
        PR_ERR("[car_control]   - Car IP address incorrect (current: %s)", s_car_ip);
        PR_ERR("[car_control]   - Car not powered on or not on network");
        PR_ERR("[car_control]   - Network connectivity issue");
        PR_ERR("[car_control] ====================================");
        rt = OPRT_COM_ERROR;
    } else if (response.status_code != 200) {
        PR_ERR("[car_control] ========== HTTP Response ==========");
        PR_ERR("[car_control] HTTP status code=%d", response.status_code);
        if (response.body && response.body_length > 0) {
            char body_preview[128];
            size_t preview_len = response.body_length < 127 ? response.body_length : 127;
            memcpy(body_preview, response.body, preview_len);
            body_preview[preview_len] = '\0';
            PR_ERR("[car_control] Response body: %s", body_preview);
        }
        PR_ERR("[car_control] ====================================");
        rt = OPRT_COM_ERROR;
    } else {
        /* Copy response body */
        size_t copy_len = response.body_length;
        if (copy_len >= buf_size) {
            copy_len = buf_size - 1;
        }
        if (response.body && copy_len > 0) {
            memcpy(response_buf, response.body, copy_len);
            response_buf[copy_len] = '\0';
        } else {
            response_buf[0] = '\0';
        }

        PR_INFO("[car_control] ========== HTTP Response ==========");
        PR_INFO("[car_control] Status: %d OK", response.status_code);
        PR_INFO("[car_control] Body: %s", response_buf);
        PR_INFO("[car_control] ====================================");
    }

    claw_free(resp_buf);
    return rt;
}

/**
 * @brief Tool: car_set_ip
 * Set the car's IP address
 */
static OPERATE_RET __tool_car_set_ip(const MCP_PROPERTY_LIST_T *properties,
                                     MCP_RETURN_VALUE_T *ret_val,
                                     void *user_data)
{
    (void)user_data;

    const char *ip = __get_str_prop(properties, "ip_address");
    if (!ip || ip[0] == '\0') {
        ai_mcp_return_value_set_str(ret_val, "Error: ip_address parameter is required");
        return OPRT_INVALID_PARM;
    }

    /* Basic IP validation */
    size_t len = strlen(ip);
    if (len >= sizeof(s_car_ip)) {
        ai_mcp_return_value_set_str(ret_val, "Error: IP address too long");
        return OPRT_INVALID_PARM;
    }

    /* Update IP */
    memcpy(s_car_ip, ip, len + 1);

    char result[128];
    snprintf(result, sizeof(result), "Car IP address set to: %s", s_car_ip);
    ai_mcp_return_value_set_str(ret_val, result);

    PR_INFO("[car_control] ====================================");
    PR_INFO("[car_control] Car IP updated to: %s", s_car_ip);
    PR_INFO("[car_control] ====================================");
    return OPRT_OK;
}

/**
 * @brief Tool: car_get_ip
 * Get the current car IP address
 */
static OPERATE_RET __tool_car_get_ip(const MCP_PROPERTY_LIST_T *properties,
                                     MCP_RETURN_VALUE_T *ret_val,
                                     void *user_data)
{
    (void)properties;
    (void)user_data;

    char result[128];
    snprintf(result, sizeof(result), "Current car IP address: %s", s_car_ip);
    ai_mcp_return_value_set_str(ret_val, result);

    PR_INFO("[car_control] Current car IP: %s", s_car_ip);
    return OPRT_OK;
}

/**
 * @brief Tool: car_command
 * Send a single command to the car
 */
static OPERATE_RET __tool_car_command(const MCP_PROPERTY_LIST_T *properties,
                                      MCP_RETURN_VALUE_T *ret_val,
                                      void *user_data)
{
    (void)user_data;

    const char *action = __get_str_prop(properties, "action");
    const char *value_str = __get_str_prop(properties, "value");

    if (!action || action[0] == '\0') {
        ai_mcp_return_value_set_str(ret_val, "Error: action parameter is required");
        return OPRT_INVALID_PARM;
    }

    /* Parse value (default to 0 if not provided) */
    int value = 0;
    if (value_str && value_str[0] != '\0') {
        value = atoi(value_str);
    }

    /* Build JSON body */
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ai_mcp_return_value_set_str(ret_val, "Error: failed to create JSON");
        return OPRT_MALLOC_FAILED;
    }

    cJSON_AddStringToObject(root, "action", action);
    cJSON_AddNumberToObject(root, "value", value);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_str) {
        ai_mcp_return_value_set_str(ret_val, "Error: failed to serialize JSON");
        return OPRT_MALLOC_FAILED;
    }

    PR_INFO("[car_control] Sending command: action=%s value=%d to %s", action, value, s_car_ip);

    /* Send HTTP POST */
    char response[256] = {0};
    OPERATE_RET rt = __car_http_post("/command", json_str, response, sizeof(response));
    cJSON_free(json_str);

    if (rt != OPRT_OK) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg),
                "Error: failed to send command to car at %s (check IP and network)", s_car_ip);
        ai_mcp_return_value_set_str(ret_val, err_msg);
        return rt;
    }

    /* Build success response */
    char result[256];
    snprintf(result, sizeof(result),
            "Command sent successfully: %s %dms (car IP: %s)", action, value, s_car_ip);
    ai_mcp_return_value_set_str(ret_val, result);

    return OPRT_OK;
}

/**
 * @brief Tool: car_sequence
 * Send a sequence of commands to the car
 */
static OPERATE_RET __tool_car_sequence(const MCP_PROPERTY_LIST_T *properties,
                                       MCP_RETURN_VALUE_T *ret_val,
                                       void *user_data)
{
    (void)user_data;

    const char *sequence_json = __get_str_prop(properties, "sequence");
    if (!sequence_json || sequence_json[0] == '\0') {
        ai_mcp_return_value_set_str(ret_val, "Error: sequence parameter is required");
        return OPRT_INVALID_PARM;
    }

    /* Validate JSON */
    cJSON *seq = cJSON_Parse(sequence_json);
    if (!seq || !cJSON_IsArray(seq)) {
        if (seq) {
            cJSON_Delete(seq);
        }
        ai_mcp_return_value_set_str(ret_val,
            "Error: sequence must be a JSON array like [{\"action\":\"forward\",\"value\":100}]");
        return OPRT_INVALID_PARM;
    }

    int count = cJSON_GetArraySize(seq);
    cJSON_Delete(seq);

    PR_INFO("[car_control] Sending sequence with %d commands to %s", count, s_car_ip);

    /* Send HTTP POST */
    char response[256] = {0};
    OPERATE_RET rt = __car_http_post("/sequence", sequence_json, response, sizeof(response));

    if (rt != OPRT_OK) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg),
                "Error: failed to send sequence to car at %s (check IP and network)", s_car_ip);
        ai_mcp_return_value_set_str(ret_val, err_msg);
        return rt;
    }

    /* Build success response */
    char result[256];
    snprintf(result, sizeof(result),
            "Sequence sent successfully: %d commands (car IP: %s)", count, s_car_ip);
    ai_mcp_return_value_set_str(ret_val, result);

    return OPRT_OK;
}

/**
 * @brief Register car control MCP tools
 */
OPERATE_RET tool_car_control_register(void)
{
    OPERATE_RET rt = OPRT_OK;

    PR_INFO("[car_control] registering tools...");

    /* Register car_set_ip tool */
    TUYA_CALL_ERR_RETURN(AI_MCP_TOOL_ADD(
        "car_set_ip",
        "Set the IP address of the ESP32 car. "
        "Default is 192.168.3.111. "
        "Call this tool when the user says something like '小车IP为xxx.xxx.xxx.xxx' or 'set car IP to xxx.xxx.xxx.xxx'.",
        __tool_car_set_ip,
        NULL,
        MCP_PROP_STR("ip_address", "The IP address of the car (e.g., 192.168.3.111)")
    ));

    /* Register car_get_ip tool */
    TUYA_CALL_ERR_RETURN(AI_MCP_TOOL_ADD(
        "car_get_ip",
        "Get the current IP address of the ESP32 car. "
        "Use this to check what IP address is currently configured.",
        __tool_car_get_ip,
        NULL
    ));

    /* Register car_command tool */
    TUYA_CALL_ERR_RETURN(AI_MCP_TOOL_ADD(
        "car_command",
        "Send a single command to the ESP32 car. "
        "Supported actions: forward (前进), backward (后退), left (左平移), right (右平移), "
        "rotate_left (右转), rotate_right (左转), stop (停止). "
        "IMPORTANT: rotate_left means turn RIGHT, rotate_right means turn LEFT (hardware is reversed). "
        "The value parameter specifies duration in milliseconds. "
        "Examples: '小车前进1000ms' -> action=forward, value=1000; '小车左转90' -> action=rotate_right, value=90.",
        __tool_car_command,
        NULL,
        MCP_PROP_STR("action", "Command action: forward, backward, left, right, rotate_left (右转), rotate_right (左转), stop"),
        MCP_PROP_STR_DEF("value", "Duration in milliseconds (default: 0)", "0")
    ));

    /* Register car_sequence tool */
    TUYA_CALL_ERR_RETURN(AI_MCP_TOOL_ADD(
        "car_sequence",
        "Send a sequence of commands to the ESP32 car. "
        "The sequence is a JSON array of command objects. "
        "Each command has 'action' and 'value' fields. "
        "Example: [{\"action\":\"forward\",\"value\":100},{\"action\":\"rotate_left\",\"value\":90}]",
        __tool_car_sequence,
        NULL,
        MCP_PROP_STR("sequence", "JSON array of commands, e.g., [{\"action\":\"forward\",\"value\":100}]")
    ));

    PR_INFO("[car_control] registration done");
    return rt;
}

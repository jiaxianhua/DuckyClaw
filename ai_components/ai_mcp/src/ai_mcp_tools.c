/**
 * @file ai_mcp_tools.c
 * @version 0.1
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#include "tal_api.h"

#include "tuya_ai_agent.h"

#include "ai_manage_mode.h"

#if defined(ENABLE_COMP_AI_AUDIO) && (ENABLE_COMP_AI_AUDIO == 1)
#include "ai_audio_player.h"
#endif

#if defined(ENABLE_COMP_AI_VIDEO) && (ENABLE_COMP_AI_VIDEO == 1)
#include "ai_video_input.h"
#include "tool_style_transfer.h"
#endif

#if defined(ENABLE_COMP_AI_PICTURE) && (ENABLE_COMP_AI_PICTURE == 1)
#include "ai_picture_output.h"
#endif

#include "ai_agent.h"
#include "ai_mcp_server.h"

#include "ai_mcp.h"
/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/


/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __get_device_info(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    cJSON *json = NULL;

    json = cJSON_CreateObject();
    if (!json) {
        PR_ERR("Create JSON object failed");
        return OPRT_MALLOC_FAILED;
    }

    // Implement device info retrieval logic here
    // Add device information
    cJSON_AddStringToObject(json, "model", PROJECT_NAME);
    cJSON_AddStringToObject(json, "serialNumber", "123456789");
    cJSON_AddStringToObject(json, "firmwareVersion", PROJECT_VERSION);

    // Set return value
    ai_mcp_return_value_set_json(ret_val, json);

    return OPRT_OK;
}
#if defined(ENABLE_COMP_AI_VIDEO) && (ENABLE_COMP_AI_VIDEO == 1)
static OPERATE_RET __take_photo(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    OPERATE_RET rt = OPRT_OK;
    uint8_t *image_data = NULL;
    uint32_t image_size = 0;

    TUYA_CALL_ERR_LOG(ai_video_display_start());

    tal_system_sleep(3000);

    rt = ai_video_get_jpeg_frame(&image_data, &image_size);
    if (OPRT_OK != rt) {
        PR_ERR("get jpeg frame err, rt:%d", rt);
        return rt;
    }

    rt = ai_mcp_return_value_set_image(ret_val, MCP_IMAGE_MIME_TYPE_JPEG, image_data, image_size);
    if (OPRT_OK != rt) {
        PR_ERR("set return image err, rt:%d", rt);
        ai_video_jpeg_image_free(&image_data);
        return rt;
    }

    ai_video_jpeg_image_free(&image_data);

    TUYA_CALL_ERR_LOG(ai_video_display_stop());

    return OPRT_OK;
}

static OPERATE_RET __style_photo_capture(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    OPERATE_RET rt = OPRT_OK;
    uint8_t *image_data = NULL;
    uint32_t image_size = 0;
    const char *style = "anime";
    const char *extra_prompt = "";

    // Parse parameters
    for (int i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "style") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            style = prop->default_val.str_val;
        } else if (strcmp(prop->name, "prompt") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {
            extra_prompt = prop->default_val.str_val;
        }
    }

    // Validate style parameter
    if (strcmp(style, "anime") != 0 &&
        strcmp(style, "cartoon") != 0 &&
        strcmp(style, "watercolor") != 0 &&
        strcmp(style, "sketch") != 0) {
        ai_mcp_return_value_set_str(ret_val,
            "Invalid style. Use: anime, cartoon, watercolor, or sketch");
        return OPRT_INVALID_PARM;
    }

    // Start camera and capture photo
    TUYA_CALL_ERR_LOG(ai_video_display_start());
    tal_system_sleep(1000);  // Camera stabilization

    rt = ai_video_get_jpeg_frame(&image_data, &image_size);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to capture photo, rt:%d", rt);
        ai_video_display_stop();
        ai_mcp_return_value_set_str(ret_val, "Photo capture failed");
        return rt;
    }

    ai_video_display_stop();

    PR_NOTICE("Photo captured: %d bytes, style: %s", image_size, style);

    // Process style transfer
    rt = tool_style_transfer_and_display(image_data, image_size, style);

    // Free the image data
    ai_video_jpeg_image_free(&image_data);

    if (rt != OPRT_OK) {
        PR_ERR("Style transfer failed, rt:%d", rt);
        ai_mcp_return_value_set_str(ret_val,
            "Photo captured but style conversion failed. Please check API configuration.");
        return rt;
    }

    // Return success message
    char result_msg[256];
    snprintf(result_msg, sizeof(result_msg),
             "Photo captured and processing %s style conversion. The styled image will display shortly.",
             style);
    ai_mcp_return_value_set_str(ret_val, result_msg);

    return OPRT_OK;
}

#if defined(ENABLE_COMP_AI_PICTURE) && (ENABLE_COMP_AI_PICTURE == 1)
static OPERATE_RET __generate_girl_avatar(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    OPERATE_RET rt = OPRT_OK;
    const char *avatar_url = "https://api.xyttkx.cn/avatar.php";

    PR_NOTICE("Generating random girl avatar");

    // Check if picture output is initialized
    if (!ai_picture_is_init()) {
        PR_ERR("Picture output not initialized");
        ai_mcp_return_value_set_str(ret_val, "Picture display system not ready");
        return OPRT_COM_ERROR;
    }

    // Start downloading and displaying the avatar
    rt = ai_picture_output_start(avatar_url);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to start picture output: %d", rt);
        ai_mcp_return_value_set_str(ret_val, "Failed to download avatar image");
        return rt;
    }

    PR_NOTICE("Started downloading girl avatar from: %s", avatar_url);

    // Return success message
    ai_mcp_return_value_set_str(ret_val,
        "Generating random girl avatar. The image will display on screen shortly.");

    return OPRT_OK;
}
#endif
#endif

#if defined(ENABLE_COMP_AI_AUDIO) && (ENABLE_COMP_AI_AUDIO == 1)
static OPERATE_RET __set_volume(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    uint32_t volume = 50; // default volume

    PR_DEBUG("__set_volume enter");

    // Parse properties to get volume
    for (int i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "volume") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            volume = prop->default_val.int_val;
            break;
        }
    }

    // FIXME: Implement actual volume setting logic here
    ai_audio_player_set_vol(volume);
    PR_DEBUG("set volume to %d", volume);

    // Set return value
    ai_mcp_return_value_set_bool(ret_val, TRUE);

    PR_DEBUG("__set_volume exit");

    return OPRT_OK;
}
#endif
static OPERATE_RET __set_mode(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    AI_CHAT_MODE_E mode = 0;
    
    ai_mode_get_curr_mode(&mode);

    // Parse properties to get volume
    for (int i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "mode") == 0 && prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            mode = prop->default_val.int_val;
            break;
        }
    }

    // Implement actual volume setting logic here
    OPERATE_RET rt = ai_mode_switch(mode);

    PR_DEBUG("set mode to %d rt:%d", mode, rt);

    // Set return value
    ai_mcp_return_value_set_bool(ret_val, (rt == OPRT_OK) ? TRUE : FALSE);

    return OPRT_OK;
}

static OPERATE_RET __ai_mcp_tools_register(void)
{
    OPERATE_RET rt = OPRT_OK;

    // device info get tool
    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_info_get",
        "Get device information such as model, serial number, and firmware version.",
        __get_device_info,
        NULL
    ), err);

#if defined(ENABLE_COMP_AI_VIDEO) && (ENABLE_COMP_AI_VIDEO == 1)
    // device camera take photo tool
    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_camera_take_photo",
        "Captures one or more photos using the device camera. Use when picture or scene "
        "change is detected, for visitor detection, or when the user asks for a photo.\n"
        "Parameters:\n"
        "- count (int): Number of photos to capture (1-10).\n"
        "Returns: Captured image(s) in Base64 format.",
        __take_photo,
        NULL,
        MCP_PROP_STR("question", "The question prompting the photo capture."),
        MCP_PROP_INT_DEF_RANGE("count", "Number of photos to capture (1-10).", 1, 1, 10)
    ), err);

    // device camera style photo tool
    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_camera_style_photo",
        "Captures a photo and converts it to artistic style (anime, cartoon, watercolor, sketch) using AI. "
        "Use ONLY when the user explicitly requests a styled photo (e.g., 'take an anime photo', 'cartoon style photo'). "
        "Call this tool ONCE per request - do NOT call it repeatedly. "
        "After calling, wait for the styled image to be returned and displayed automatically.\n"
        "Parameters:\n"
        "- style (string): Style type - anime, cartoon, watercolor, or sketch.\n"
        "- prompt (string, optional): Additional style instructions.\n"
        "Returns: Status message. The styled image will arrive asynchronously and display automatically.",
        __style_photo_capture,
        NULL,
        MCP_PROP_STR("style", "Style type: anime, cartoon, watercolor, or sketch"),
        MCP_PROP_STR_DEF("prompt", "Additional style instructions (optional)", "")
    ), err);
#endif

#if defined(ENABLE_COMP_AI_PICTURE) && (ENABLE_COMP_AI_PICTURE == 1)
    // generate girl avatar tool
    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_generate_girl_avatar",
        "Generates and displays a random girl avatar image on the screen. "
        "Use when the user asks for a girl avatar, female avatar, or random girl picture. "
        "The avatar is randomly selected from a curated collection of 2000+ images.\n"
        "Parameters: None\n"
        "Returns: Status message. The avatar image will be downloaded and displayed automatically.",
        __generate_girl_avatar,
        NULL
    ), err);
#endif
#endif

#if defined(ENABLE_COMP_AI_AUDIO) && (ENABLE_COMP_AI_AUDIO == 1)
    // set volume tool
    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_audio_volume_set",
        "Sets the device's volume level.\n"
        "Parameters:\n"
        "- volume (int): The volume level to set (0-100).\n"
        "Response:\n"
        "- Returns true if the volume was set successfully.",
        __set_volume,
        NULL,
        MCP_PROP_INT_RANGE("volume", "The volume level to set (0-100).", 0, 100)
    ), err);
#endif

    TUYA_CALL_ERR_GOTO(AI_MCP_TOOL_ADD(
        "device_audio_mode_set",
        "Sets the device's chat mode.\n"
        "Parameters:\n"
        "- mode (integer): The chat mode (0=hold, 1=key_press, 2=wakeup, 3=free).\n"
        "Response:\n"
        "- Returns true if the mode was set successfully.",
        __set_mode,
        NULL,
        MCP_PROP_INT_RANGE("mode", "The chat mode (0=hold, 1=key_press, 2=wakeup, 3=free)", 0, 3)
    ), err);

    return OPRT_OK;

err:
    // destroy MCP server on failure
    ai_mcp_server_destroy();

    return rt;
}

static OPERATE_RET __ai_mcp_init(void *data)
{
    OPERATE_RET rt = OPRT_OK;

    // FIXME: Set actual MCP server name and mcp version
    TUYA_CALL_ERR_RETURN(ai_mcp_server_init("Tuya MCP Server", "1.0"));

    TUYA_CALL_ERR_RETURN(__ai_mcp_tools_register());

    PR_DEBUG("MCP Server initialized successfully");

    return rt;
}

OPERATE_RET ai_mcp_init(void)
{
    return tal_event_subscribe(EVENT_MQTT_CONNECTED, "ai_mcp_init", __ai_mcp_init, SUBSCRIBE_TYPE_ONETIME);
}

OPERATE_RET ai_mcp_deinit(void)
{
    ai_mcp_server_destroy();

    PR_DEBUG("MCP Server deinitialized successfully");

    return OPRT_OK;
}
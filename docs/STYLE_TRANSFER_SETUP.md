# 图片风格转换功能配置指南

## 概述

DuckyClaw的图片风格转换功能支持将拍摄的照片转换为动漫、卡通、水彩、素描等艺术风格。

## 配置方法

### 方案1：使用DeepAI免费API（推荐）

DeepAI提供免费的图片风格转换API，每月500次免费请求。

**步骤：**

1. 注册DeepAI账号：https://deepai.org/
2. 获取API Key：https://deepai.org/dashboard/profile
3. 在 `include/tuya_app_config_secrets.h` 中添加：

```c
#define DEEPAI_API_KEY "your-api-key-here"
```

4. 重新编译并烧录固件

**支持的风格：**
- anime（动漫）
- cartoon（卡通）
- watercolor（水彩）
- sketch（素描）

### 方案2：使用Replicate API

Replicate提供更多AI模型选择，有免费额度。

**步骤：**

1. 注册Replicate账号：https://replicate.com/
2. 获取API Token
3. 配置类似DeepAI

### 方案3：本地处理（最佳性能）

通过OpenClaw网关运行本地AI模型，无需外部API。

**优势：**
- 完全免费
- 更快的处理速度
- 更好的隐私保护
- 无网络依赖

**实现方式：**
1. 在OpenClaw网关上部署风格转换模型
2. 通过 `openclaw_ctrl` MCP工具调用
3. 参考 `tools/tool_openclaw_ctrl.c`

## 使用方法

### 通过语音

说："拍一张动漫风格的照片"

### 通过飞书/IM

发送消息："拍一张卡通风格的照片"

### 通过MCP工具

```json
{
  "tool": "device_camera_style_photo",
  "parameters": {
    "style": "anime",
    "prompt": "vibrant colors"
  }
}
```

## 故障排除

### 问题：提示"API key not configured"

**解决：** 确保在 `tuya_app_config_secrets.h` 中配置了 `DEEPAI_API_KEY`

### 问题：转换失败或超时

**可能原因：**
1. 网络连接问题
2. API配额用完
3. 图片太大（建议<1MB）

**解决：**
- 检查网络连接
- 查看DeepAI账户配额
- 降低相机分辨率

### 问题：转换效果不理想

**建议：**
- 尝试不同的style参数
- 使用prompt参数添加额外描述
- 考虑使用本地模型获得更好控制

## 免费API对比

| 服务 | 免费额度 | 速度 | 质量 | 推荐度 |
|------|---------|------|------|--------|
| DeepAI | 500次/月 | 快 | 中 | ⭐⭐⭐⭐ |
| Replicate | $5信用 | 中 | 高 | ⭐⭐⭐⭐⭐ |
| 本地模型 | 无限 | 最快 | 可调 | ⭐⭐⭐⭐⭐ |

## 未来计划

- [ ] 支持更多免费API服务
- [ ] 集成Hugging Face Inference API
- [ ] 本地TinyML模型支持
- [ ] 批量处理多张照片
- [ ] 风格强度调节
- [ ] 保存转换后的图片

## 技术细节

### 数据流

```
用户请求 → MCP工具 → 拍照 → 上传到API → 下载结果 → 显示
```

### 相关文件

- `tools/tool_style_transfer.c` - 风格转换核心逻辑
- `ai_components/ai_mcp/src/ai_mcp_tools.c` - MCP工具实现
- `ai_components/ai_picture/src/ai_picture_output.c` - 图片下载和显示

### 内存使用

- 拍照JPEG：50-100KB
- API上传：流式传输
- 下载结果：50-150KB
- RGB565显示：460KB（PSRAM）

## 贡献

欢迎提交PR添加更多免费API支持或本地模型集成！

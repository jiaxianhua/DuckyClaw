# 女生头像功能配置说明

## 重要：必须启用AI_PICTURE组件

女生头像功能依赖 `AI_PICTURE` 组件，必须在配置中启用。

## 配置步骤

### 方法1：手动编辑配置文件

编辑 `app_default.config`，添加：

```
CONFIG_ENABLE_COMP_AI_PICTURE=y
```

完整示例：
```
CONFIG_ENABLE_COMP_AI_AUDIO_CODEC_OPUS=y
CONFIG_ENABLE_COMP_AI_VIDEO=y
CONFIG_ENABLE_COMP_AI_PICTURE=y          # 添加这一行
CONFIG_ENABLE_AI_UI_TEXT_STREAMING=y
```

### 方法2：使用配置工具

```bash
cd TuyaOpen
python3 tos.py menuconfig
```

导航到：
```
Components → AI Components → Enable AI Picture Component
```

按 `Y` 启用，然后保存退出。

### 方法3：使用预设配置

如果使用T5AI板子，确保使用正确的配置：

```bash
cp config/TUYA_T5AI_BOARD_LCD_3.5_CAMERA.config app_default.config
```

然后添加：
```bash
echo "CONFIG_ENABLE_COMP_AI_PICTURE=y" >> app_default.config
```

## 验证配置

编译后，检查串口日志应该看到：

```
[ai_mcp_server] Added tool: device_generate_girl_avatar
```

如果没有看到这一行，说明配置没有生效。

## 为什么需要这个配置？

`CONFIG_ENABLE_COMP_AI_PICTURE=y` 启用以下组件：

1. **ai_picture_output** - 从URL下载图片
2. **ai_picture_convert** - 图片格式转换（JPEG→RGB565）
3. **ai_ui_disp_picture** - 在LCD上显示图片

没有这些组件，女生头像工具无法：
- 下载图片
- 转换格式
- 显示到屏幕

## 故障排除

### 问题：工具没有注册

**症状：** 说"女生头像"时，AI调用 `device_camera_take_photo` 而不是 `device_generate_girl_avatar`

**原因：** `CONFIG_ENABLE_COMP_AI_PICTURE` 未启用

**解决：** 
1. 添加配置
2. 重新编译
3. 重新烧录

### 问题：编译错误

**症状：** 编译时提示找不到 `ai_picture_output.h`

**原因：** 配置文件格式错误或路径问题

**解决：**
1. 检查配置文件语法
2. 确保没有多余空格
3. 重新运行 `tos.py build`

## 完整编译流程

```bash
# 1. 确保配置正确
grep "CONFIG_ENABLE_COMP_AI_PICTURE" app_default.config
# 应该输出：CONFIG_ENABLE_COMP_AI_PICTURE=y

# 2. 清理并重新编译
cd TuyaOpen
python3 tos.py clean
python3 tos.py build

# 3. 烧录
python3 tos.py flash

# 4. 监控日志
python3 tos.py monitor

# 5. 验证工具已注册
# 在日志中搜索：device_generate_girl_avatar
```

## 注意事项

- `app_default.config` 通常在 `.gitignore` 中，不会提交到git
- 每次切换板子配置后，需要重新添加这一行
- 建议将配置保存到 `config/` 目录下的板子配置文件中

## 相关配置

其他可能需要的配置：

```
CONFIG_ENABLE_COMP_AI_VIDEO=y          # 摄像头功能
CONFIG_ENABLE_COMP_AI_DISPLAY=y        # 显示功能
CONFIG_ENABLE_EX_MODULE_CAMERA=y       # 外部摄像头模块
CONFIG_TUYA_T5AI_BOARD_EX_MODULE_35565LCD=y  # 3.5寸LCD
```

这些配置通常已经在板子配置文件中启用。

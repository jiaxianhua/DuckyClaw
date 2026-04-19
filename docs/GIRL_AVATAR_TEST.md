# 女生头像功能测试指南

## 快速测试步骤

### 1. 编译和烧录

```bash
cd TuyaOpen
source .venv/bin/activate
python3 tos.py build
python3 tos.py flash
python3 tos.py monitor
```

### 2. 测试方法

#### 方法1：语音测试（推荐）

1. 按住按钮说话
2. 说："生成女生头像"或"女生头像"
3. 松开按钮
4. 等待2-5秒，屏幕应显示随机女生头像

#### 方法2：飞书测试

1. 在飞书中发送消息："女生头像"
2. 等待设备响应
3. 屏幕应显示头像

#### 方法3：连续测试

- 说："再来一张"
- 每次都会显示不同的随机头像

### 3. 预期结果

**成功标志：**
- ✅ 串口日志显示：`Generating random girl avatar`
- ✅ 串口日志显示：`Started downloading girl avatar from: https://api.xyttkx.cn/avatar.php`
- ✅ 串口日志显示：`Picture convert success`
- ✅ LCD屏幕显示女生头像图片

**失败排查：**
- ❌ 如果提示"Picture display system not ready"
  - 检查 `CONFIG_ENABLE_COMP_AI_PICTURE=y` 是否启用
  
- ❌ 如果下载失败
  - 检查WiFi连接
  - 检查网络是否能访问外网
  
- ❌ 如果图片不显示
  - 检查LCD是否正常工作
  - 查看串口日志中的错误信息

### 4. 串口日志示例

成功的日志应该类似：

```
[ai_mcp] Tool executed: device_generate_girl_avatar
[ai_mcp_tools] Generating random girl avatar
[ai_mcp_tools] Started downloading girl avatar from: https://api.xyttkx.cn/avatar.php
[ai_picture_output] Downloading image from URL
[ai_picture_output] download image complete, size: 45678
[ai_picture_convert] Picture convert success: fmt=RGB565, width=480, height=480
[ai_ui] Picture displayed: 480x480
```

### 5. 性能指标

- **下载时间**: 1-3秒
- **转换时间**: <1秒
- **总耗时**: 2-5秒
- **内存使用**: ~500KB

### 6. 多次测试

建议测试5-10次，验证：
- 每次都能成功显示
- 每次显示的头像都不同
- 没有内存泄漏（查看Free heap日志）

### 7. 故障排除命令

```bash
# 查看WiFi状态
! iwconfig

# 测试网络连接
! ping -c 3 api.xyttkx.cn

# 查看内存使用
# 在串口日志中搜索 "Free heap size"
```

## 已知问题

1. **首次下载可能较慢**
   - 原因：DNS解析和TLS握手
   - 解决：第二次会快很多

2. **偶尔下载失败**
   - 原因：网络波动或API暂时不可用
   - 解决：重试即可

3. **图片质量不一**
   - 原因：API返回的图片质量由源决定
   - 说明：这是正常现象

## 成功案例

如果一切正常，你应该看到：
1. 语音识别成功
2. AI理解意图并调用工具
3. 下载进度日志
4. 图片转换日志
5. LCD显示头像
6. AI语音回复："已为您生成女生头像"

祝测试顺利！🎉

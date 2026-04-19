# 小车控制功能

DuckyClaw 现在可以通过 HTTP 命令控制基于 ESP32 的小车。

## 功能说明

### 1. 设置小车 IP 地址

**命令示例：**
- "小车IP为192.168.3.100"
- "设置小车IP为192.168.3.111"

默认 IP：192.168.3.111

### 2. 单个命令控制

**支持的动作：**
- `forward` (前进)
- `backward` (后退)
- `left` (左平移)
- `right` (右平移)
- `rotate_left` (右转) - **注意：硬件反向，实际是右转**
- `rotate_right` (左转) - **注意：硬件反向，实际是左转**
- `stop` (停止)

**重要提示：旋转方向映射**
- 用户说"左转" → AI 会发送 `rotate_right` 命令
- 用户说"右转" → AI 会发送 `rotate_left` 命令

**命令示例：**
- "小车前进1000ms" → 前进1000毫秒
- "小车后退500" → 后退500毫秒
- "小车左转90" → 发送 rotate_right，实际左转90毫秒
- "小车右转90" → 发送 rotate_left，实际右转90毫秒
- "小车停止" → 停止

### 3. 命令序列

可以让小车执行一系列动作。

**命令示例：**
- "小车先前进100ms，然后左转90度，再前进50ms"

## ESP32 端要求

ESP32 需要运行 WebServer，提供以下接口：

1. `POST /command` - 执行单个命令
   ```json
   {"action":"forward","value":1000}
   ```

2. `POST /sequence` - 执行命令序列
   ```json
   [
     {"action":"forward","value":100},
     {"action":"rotate_left","value":90},
     {"action":"forward","value":50}
   ]
   ```

## 实现细节

- **文件位置：**
  - `tools/tool_car_control.c` - 主实现
  - `tools/tool_car_control.h` - 头文件
  - `tools/tools_register.c` - 工具注册

- **MCP 工具：**
  - `car_set_ip` - 设置IP地址
  - `car_command` - 发送单个命令
  - `car_sequence` - 发送命令序列

- **HTTP 客户端：** 使用 TuyaOpen 的 `http_client_interface`
- **超时设置：** 5000ms
- **端口：** 80 (HTTP)

## 故障排查

1. **连接失败** - 检查小车IP是否正确，小车是否开机
2. **网络错误** - 确保 DuckyClaw 和小车在同一 WiFi 网络
3. **命令无效** - 验证 ESP32 WebServer 是否正常运行

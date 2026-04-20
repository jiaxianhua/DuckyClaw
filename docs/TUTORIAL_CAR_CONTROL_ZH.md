# 使用 TuyaT5AIBoard 3.5LCD 控制智能小车

> 基于 DuckyClaw AI Agent 的语音控制智能小车教程

## 目录

- [项目简介](#项目简介)
- [硬件准备](#硬件准备)
- [软件架构](#软件架构)
- [快速开始](#快速开始)
- [功能说明](#功能说明)
- [进阶使用](#进阶使用)
- [故障排查](#故障排查)
- [技术原理](#技术原理)

---

## 项目简介

本项目实现了使用 **TuyaT5AIBoard** 开发板通过语音命令控制 ESP32 智能小车。用户可以通过自然语言与 AI 助手对话，AI 助手会理解用户意图并发送 HTTP 命令到小车，实现前进、后退、转向等操作。

### 核心特性

- ✅ **语音控制**：通过自然语言控制小车（"小车前进1000ms"）
- ✅ **AI 理解**：智能理解复杂指令（"小车先前进100，然后左转90，再前进50"）
- ✅ **实时反馈**：AI 语音播报执行结果
- ✅ **灵活配置**：支持动态设置小车 IP 地址
- ✅ **命令序列**：支持执行多个连续动作

### 技术栈

- **硬件**：TuyaT5AIBoard (带 3.5 寸 LCD 屏幕)
- **小车**：ESP32 开发板 + 电机驱动
- **AI 框架**：DuckyClaw (基于 TuyaOpen SDK)
- **通信协议**：HTTP/JSON
- **AI 模型**：云端大语言模型

---

## 硬件准备

### 1. TuyaT5AIBoard 开发板

**规格要求**：
- 型号：TuyaT5AIBoard
- 屏幕：3.5 寸 LCD
- 网络：WiFi 连接
- 音频：麦克风 + 扬声器

**配置文件**：`config/TUYA_T5AI_BOARD_LCD_3.5_CAMERA.config`

### 2. ESP32 智能小车

**硬件组成**：
- ESP32 开发板（ESP32-S3 推荐）
- 电机驱动模块（L298N 或 TB6612）
- 4 个直流电机
- 电池供电系统
- 车体底盘

**软件要求**：
- WebServer 库
- HTTP API 支持
- 支持 `/command` 和 `/sequence` 端点

### 3. 网络环境

- 确保 TuyaT5AIBoard 和 ESP32 小车在同一 WiFi 网络
- 路由器支持设备间通信（非 AP 隔离模式）

---

## 软件架构

### 系统架构图

```
┌─────────────────┐      语音输入      ┌──────────────────┐
│   用户语音命令   │ ───────────────> │  TuyaT5AIBoard   │
│  "小车前进1000" │                   │   (DuckyClaw)    │
└─────────────────┘                   └──────────────────┘
                                              │
                                              │ AI 理解
                                              ▼
                                      ┌──────────────────┐
                                      │   AI Agent       │
                                      │  (云端大模型)     │
                                      └──────────────────┘
                                              │
                                              │ MCP Tool 调用
                                              ▼
                                      ┌──────────────────┐
                                      │  car_command     │
                                      │  (HTTP Client)   │
                                      └──────────────────┘
                                              │
                                              │ HTTP POST
                                              ▼
                                      ┌──────────────────┐
                                      │  ESP32 小车      │
                                      │  (WebServer)     │
                                      └──────────────────┘
                                              │
                                              │ 执行动作
                                              ▼
                                      ┌──────────────────┐
                                      │   电机控制       │
                                      │   前进/后退/转向  │
                                      └──────────────────┘
```

### 核心组件

#### 1. DuckyClaw Agent Loop
- 监听用户语音输入
- 调用云端 AI 模型理解意图
- 执行 MCP 工具
- 语音播报结果

#### 2. Car Control Skill
- 定义小车控制的触发条件
- 指导 AI 如何使用工具
- 处理左转/右转的硬件映射

#### 3. MCP Tools
- `car_set_ip`: 设置小车 IP
- `car_get_ip`: 查询当前 IP
- `car_command`: 发送单个命令
- `car_sequence`: 发送命令序列

#### 4. ESP32 WebServer
- 接收 HTTP POST 请求
- 解析 JSON 命令
- 控制电机执行动作

---

## 快速开始

### 步骤 1: 准备 ESP32 小车

确保你的 ESP32 小车已经烧录了 WebServer 程序，并支持以下 API：

**单个命令接口**：
```bash
POST http://192.168.3.115/command
Content-Type: application/json

{
  "action": "forward",
  "value": 1000
}
```

**命令序列接口**：
```bash
POST http://192.168.3.115/sequence
Content-Type: application/json

[
  {"action": "forward", "value": 100},
  {"action": "rotate_left", "value": 90},
  {"action": "forward", "value": 50}
]
```

### 步骤 2: 编译 DuckyClaw

```bash
# 进入 TuyaOpen 目录并初始化环境
cd TuyaOpen
source ./export.sh
cd ..

# 选择 TuyaT5AIBoard 配置
cp config/TUYA_T5AI_BOARD_LCD_3.5_CAMERA.config app_default.config

# 编译项目
cd TuyaOpen
python3 tos.py build

# 烧录到开发板
python3 tos.py flash
```

### 步骤 3: 配置网络和密钥

编辑 `include/tuya_app_config_secrets.h`：

```c
// Tuya 云平台配置
#define TUYA_PRODUCT_ID "your_product_id"
#define TUYA_OPENSDK_UUID "your_uuid"
#define TUYA_OPENSDK_AUTHKEY "your_authkey"

// WiFi 配置（如果需要）
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

### 步骤 4: 启动并配置小车 IP

1. 启动 TuyaT5AIBoard，等待连接到云端
2. 对着麦克风说："小车IP为192.168.3.115"
3. AI 会确认："Car IP address set to: 192.168.3.115"

### 步骤 5: 开始控制小车

现在你可以通过语音控制小车了：

- "小车前进1000毫秒"
- "小车后退500"
- "小车左转90度"
- "小车停止"

---

## 功能说明

### 支持的命令

#### 1. 基础移动命令

| 命令示例 | 动作 | 参数说明 |
|---------|------|---------|
| "小车前进1000ms" | 前进 | 持续时间（毫秒） |
| "小车后退500" | 后退 | 持续时间（毫秒） |
| "小车左平移200" | 左平移 | 持续时间（毫秒） |
| "小车右平移200" | 右平移 | 持续时间（毫秒） |
| "小车停止" | 停止 | 无 |

#### 2. 旋转命令

⚠️ **重要提示**：由于硬件接线原因，左转和右转的命令是反向的：

| 用户说 | AI 发送的命令 | 实际动作 |
|-------|-------------|---------|
| "小车左转90" | `rotate_right` | 左转 |
| "小车右转90" | `rotate_left` | 右转 |

AI 会自动处理这个映射，用户只需要说"左转"或"右转"即可。

#### 3. 复杂命令序列

AI 可以理解复杂的连续动作：

**示例 1**：
```
用户："小车先前进100毫秒，然后左转90度，再前进50毫秒"
```

AI 会自动拆解为：
```json
[
  {"action": "forward", "value": 100},
  {"action": "rotate_right", "value": 90},
  {"action": "forward", "value": 50}
]
```

**示例 2**：
```
用户："让小车画一个正方形"
```

AI 会生成：
```json
[
  {"action": "forward", "value": 1000},
  {"action": "rotate_right", "value": 90},
  {"action": "forward", "value": 1000},
  {"action": "rotate_right", "value": 90},
  {"action": "forward", "value": 1000},
  {"action": "rotate_right", "value": 90},
  {"action": "forward", "value": 1000}
]
```

#### 4. IP 地址管理

| 命令 | 功能 |
|------|------|
| "小车IP为192.168.3.115" | 设置小车 IP 地址 |
| "小车当前IP是多少" | 查询当前配置的 IP |

---

## 进阶使用

### 1. 查看详细日志

DuckyClaw 会在 `monitor.log` 中记录所有 HTTP 请求和响应：

```bash
tail -f monitor.log | grep car_control
```

日志示例：
```
[car_control] ========== HTTP Request ==========
[car_control] URL: http://192.168.3.115:80/command
[car_control] Method: POST
[car_control] Body: {"action":"forward","value":1000}
[car_control] ====================================
[car_control] ========== HTTP Response ==========
[car_control] Status: 200 OK
[car_control] Body: {"status":"ok","commands":1}
[car_control] ====================================
```

### 2. 自定义 Skill

你可以修改 `skills/skill_loader.c` 中的 `BUILTIN_CAR_CONTROL` 来自定义 AI 的行为：

```c
#define BUILTIN_CAR_CONTROL \
    "# Car Control\n" \
    "...\n" \
    "## Custom behavior\n" \
    "- Add your custom instructions here\n"
```

### 3. 添加新的动作

在 ESP32 端添加新的动作支持，然后更新 DuckyClaw 的工具描述：

```c
// 在 tool_car_control.c 中更新描述
"Supported actions: forward, backward, left, right, rotate_left, rotate_right, stop, dance"
```

### 4. 使用命令行测试

你可以使用 `curl` 直接测试小车：

```bash
# 测试单个命令
curl -X POST http://192.168.3.115/command \
  -H "Content-Type: application/json" \
  -d '{"action":"forward","value":1000}'

# 测试命令序列
curl -X POST http://192.168.3.115/sequence \
  -H "Content-Type: application/json" \
  -d '[{"action":"forward","value":100},{"action":"rotate_left","value":90}]'
```

---

## 故障排查

### 问题 1: "小车没有响应"

**可能原因**：
1. IP 地址配置错误
2. 小车未开机或未连接到 WiFi
3. 网络不通

**解决方法**：
```bash
# 1. 检查当前配置的 IP
对 AI 说："小车当前IP是多少"

# 2. 重新设置正确的 IP
对 AI 说："小车IP为192.168.3.115"

# 3. 测试网络连通性
ping 192.168.3.115

# 4. 查看详细日志
tail -f monitor.log | grep car_control
```

### 问题 2: "左转和右转反了"

这是正常的！由于硬件接线原因，代码中已经做了映射：
- 用户说"左转" → 发送 `rotate_right` → 实际左转
- 用户说"右转" → 发送 `rotate_left` → 实际右转

如果你的小车方向是正确的，需要修改 `tool_car_control.c` 中的映射关系。

### 问题 3: "HTTP 请求超时"

**检查项**：
1. 小车的 WebServer 是否正常运行
2. 防火墙是否阻止了连接
3. WiFi 信号是否稳定

**调整超时时间**：
```c
// 在 tool_car_control.c 中修改
#define CAR_HTTP_TIMEOUT_MS 10000  // 增加到 10 秒
```

### 问题 4: "AI 不理解我的命令"

**优化建议**：
1. 使用清晰的命令："小车前进1000毫秒"
2. 避免模糊的表达："让小车动一下"
3. 查看 Skill 定义，了解 AI 支持的命令格式

---

## 技术原理

### 1. SKILL vs MCP Tool

**SKILL（技能文档）**：
- 存储在 `skills/skill_loader.c` 中
- 是给 AI 的"使用说明书"
- 告诉 AI 什么时候、如何使用工具
- 不执行任何代码

**MCP Tool（工具）**：
- 实现在 `tools/tool_car_control.c` 中
- 是实际执行操作的 C 代码
- 发送 HTTP 请求到 ESP32
- 返回执行结果

**执行流程**：
```
用户语音 → AI 读取 SKILL → AI 决定调用哪个 Tool → Tool 执行 → 返回结果
```

### 2. HTTP 通信协议

DuckyClaw 使用 TuyaOpen 的 `http_client_interface` 发送 HTTP 请求：

```c
http_client_request_t request = {
    .host = "192.168.3.115",
    .port = 80,
    .path = "/command",
    .method = "POST",
    .headers = headers,
    .headers_count = 1,
    .body = (const uint8_t *)json_body,
    .body_length = strlen(json_body),
    .timeout_ms = 5000
};
```

### 3. JSON 数据格式

**单个命令**：
```json
{
  "action": "forward",
  "value": 1000
}
```

**命令序列**：
```json
[
  {"action": "forward", "value": 100},
  {"action": "rotate_right", "value": 90},
  {"action": "forward", "value": 50}
]
```

### 4. 左转/右转映射原理

由于硬件接线原因，电机的旋转方向可能与预期相反。DuckyClaw 在工具描述中明确告诉 AI：

```c
"rotate_left (右转) - Turn RIGHT (hardware reversed)"
"rotate_right (左转) - Turn LEFT (hardware reversed)"
```

AI 会自动处理这个映射，用户无需关心底层细节。

### 5. 内存管理

DuckyClaw 使用 PSRAM-aware 的内存分配：

```c
// 自动选择 PSRAM 或普通 RAM
uint8_t *resp_buf = (uint8_t *)claw_malloc(CAR_RESP_BUF_SIZE);

// 使用完毕后释放
claw_free(resp_buf);
```

### 6. 错误处理

完整的错误处理流程：

```c
http_client_status_t status = http_client_request(&request, &response);

if (status != HTTP_CLIENT_SUCCESS) {
    PR_ERR("[car_control] HTTP request failed, status=%d", status);
    return OPRT_COM_ERROR;
} else if (response.status_code != 200) {
    PR_ERR("[car_control] HTTP status code=%d", response.status_code);
    return OPRT_COM_ERROR;
}
```

---

## 代码结构

```
DuckyClaw/
├── tools/
│   ├── tool_car_control.c      # 小车控制工具实现
│   ├── tool_car_control.h      # 头文件
│   └── tools_register.c        # 工具注册
├── skills/
│   └── skill_loader.c          # Skill 定义（包含 car-control）
├── docs/
│   ├── CAR_CONTROL_ZH.md       # 功能说明文档
│   ├── SKILL_CAR_CONTROL.md    # Skill 文档
│   └── TUTORIAL_CAR_CONTROL_ZH.md  # 本教程
└── config/
    └── TUYA_T5AI_BOARD_LCD_3.5_CAMERA.config  # 开发板配置
```

---

## 扩展阅读

### 相关文档

- [DuckyClaw 项目文档](../README.md)
- [小车控制功能说明](CAR_CONTROL_ZH.md)
- [Skill 开发指南](SKILL_CAR_CONTROL.md)
- [TuyaOpen SDK 文档](../TuyaOpen/README.md)

### 参考资源

- [Datawhale AI 教程](https://www.datawhale.cn/learn/content/268/6041)
- [ESP32 WebServer 开发](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html)
- [HTTP Client Interface](../TuyaOpen/src/libhttp/include/http_client_interface.h)

---

## 常见问题 FAQ

**Q: 可以同时控制多个小车吗？**  
A: 目前只支持一个小车。如果需要控制多个小车，可以扩展代码支持多个 IP 地址。

**Q: 支持哪些语言？**  
A: 主要支持中文，也支持英文命令（如 "car move forward 1000ms"）。

**Q: 可以通过手机控制吗？**  
A: 可以！DuckyClaw 支持通过 Telegram、Discord、飞书等 IM 平台发送文字命令。

**Q: 延迟有多大？**  
A: 从语音输入到小车执行，通常在 2-3 秒左右（包括语音识别、AI 理解、网络传输）。

**Q: 可以离线使用吗？**  
A: 不可以。DuckyClaw 依赖云端 AI 模型进行语音识别和意图理解。

---

## 贡献与反馈

如果你在使用过程中遇到问题或有改进建议，欢迎：

1. 提交 Issue 到项目仓库
2. 查看 `monitor.log` 日志并提供详细信息
3. 参考故障排查章节自行解决

---

## 许可证

本项目基于 TuyaOpen SDK 开发，遵循相应的开源协议。

---

**祝你玩得开心！🚗💨**


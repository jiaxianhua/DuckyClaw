# DuckyClaw 项目 Code Wiki

## 1. 项目概述

DuckyClaw 是一个硬件导向的 AI 代理项目，运行在边缘设备上，通过自然语言与物联网设备交互。其核心价值在于让用户通过 IM 渠道（Telegram / Discord / Feishu）使用自然语言与 IoT 设备进行交互。设备上的代理接收指令，调用本地 MCP 工具执行实际操作，并通过同一渠道回复。

### 主要特性
- **统一多渠道**：Telegram、Discord、Feishu、WebSocket、串行 CLI 共享单一代理循环
- **工具循环**：单个用户消息最多可触发 10 次 LLM ↔ 工具迭代，直到代理产生最终回复
- **持久内存**：长期记忆（MEMORY.md）、每日笔记（YYYY-MM-DD.md）、个性（SOUL.md）、用户配置文件（USER.md）存储在闪存/SD 卡上
- **跨平台部署**：通过条件编译，同一代码库可在 Tuya T5AI、ESP32-S3、Raspberry Pi 和 Linux 桌面运行

## 2. 系统架构

### 2.1 分层架构

```
┌──────────────────────────────────────────────────────┐
│                     IM Channel Layer                  │
│  Telegram  │  Discord  │  Feishu  │  WebSocket │ CLI │
└─────────────────────┬────────────────────────────────┘
                      │ im_msg_t (inbound / outbound)
              ┌───────▼───────┐
              │  Message Bus  │  Thread-safe dual queue
              └───────┬───────┘
                      │
              ┌───────▼───────┐
              │  Agent Loop   │  Outer: wait for msg
              │               │  Inner: ≤10 tool iterations
              └───┬───────┬───┘
                  │       │
        ┌─────────▼─┐ ┌──▼──────────┐
        │ Context   │ │ Cloud AI    │
        │ Builder   │ │ (ai_agent)  │
        │ sys prompt│ │ stream cb   │
        └─────┬─────┘ └──────┬──────┘
              │              │
    ┌─────────▼──────────────▼──────────┐
    │           MCP Tools Layer          │
    │ files │ cron │ exec │ openclaw    │
    └─────────┬──────────────┬──────────┘
              │              │
    ┌─────────▼─────┐  ┌────▼─────────┐
    │   Memory /    │  │   Gateway    │
    │   Session     │  │  WS + ACP    │
    └───────────────┘  └──────────────┘
```

### 2.2 核心数据流

1. **入站**：IM 通道（或 WS/cron/ACP）构建 `im_msg_t` → `message_bus_push_inbound()`
2. **代理消费**：`agent_loop_task` 阻塞等待 `message_bus_pop_inbound()`
3. **内循环**：`context_build_system_prompt()` 组装系统提示 + 历史记录 → `ai_agent_send_text()` 发送到云端
4. **AI 回调**：`ducky_claw_chat.c` 的 `__ai_chat_handle_event` 接收流事件：
   - `STREAM_START/DATA/STOP`：累积文本 → 记录到历史记录
   - `END`：调用 `agent_loop_set_last_response()` + `agent_loop_notify_turn_done()`（发布信号量）
5. **工具执行**：云端 AI 触发 MCP 工具调用 → `__on_tool_executed` 钩子记录结果 → 设置 `s_turn.tool_called = true`
6. **循环决策**：信号量发布后，代理检查 `tool_called` — 如果为 true，将工具结果作为下一个输入；否则将最终回复转发到出站队列
7. **出站**：`outbound_dispatch_task` 出队消息 → 通过 `channel` 字段调度到相应的 IM SDK

### 2.3 线程模型

| 线程 | 职责 | 入口 |
|------|------|------|
| `tuya_app_main` / `main` | SDK 初始化 + `tuya_iot_yield` 主循环 | `user_main()` |
| `agent_loop` | 外循环 + 内工具迭代 | `agent_loop_task()` |
| `outbound_loop` | 出站消息调度 | `outbound_dispatch_task()` |
| `ws_server` | WebSocket 服务器 | `ws_server.c` |
| `acp_client` | OpenClaw 网关 WS 客户端 | `acp_client.c` |
| `cron_service` | 调度任务调度器 | `cron_service.c` |
| IM 通道线程 | Telegram 轮询 / Discord 网关 / Feishu WS | `*_bot.c` |

### 2.4 同步机制

- **代理循环 ↔ AI 回调**：二元信号量 `s_turn.sem`（`agent_loop_task` 等待，`ducky_claw_chat` 发布）
- **共享历史**：`s_history_mutex` 保护 `s_history_json`（cJSON 数组，滑动窗口 ≤ 10 个条目）
- **工具状态**：`s_turn.lock` 保护 `tool_called` / `tool_result`
- **消息总线**：`tal_queue` 本身是线程安全的

## 3. 核心模块

### 3.1 Agent 模块

**Agent 模块**是整个系统的核心，负责协调用户输入、AI 推理和工具执行。

#### 3.1.1 agent_loop

**功能**：实现代理的主循环，包括外循环（等待用户消息）和内循环（工具迭代）。

**主要组件**：
- `agent_loop_task()`：外循环，等待入站消息并启动内循环
- `__build_and_send()`：构建完整提示并发送到云端 AI
- `__on_tool_executed()`：MCP 工具执行钩子，记录结果
- `build_current_context()`：将角色/内容对追加到共享历史记录

**关键流程**：
1. 从消息总线获取用户消息
2. 记录用户消息到历史记录
3. 进入内循环（最多 10 次迭代）
4. 构建提示并发送到 AI
5. 等待 AI 完成（包括工具调用）
6. 检查是否调用了工具
7. 如果调用了工具，将工具结果作为下一次迭代的输入
8. 如果未调用工具，将最终回复发送到 IM

#### 3.1.2 context_builder

**功能**：构建系统提示，包括规则、记忆、技能和个性。

**主要组件**：
- `context_build_system_prompt()`：构建完整的系统提示
- `append_file()`：将文件内容追加到提示中

**关键流程**：
1. 构建基础提示（角色、规则、可用工具）
2. 追加个性文件（SOUL.md）
3. 追加用户信息文件（USER.md）
4. 追加长期记忆
5. 追加最近的每日笔记
6. 追加技能摘要

### 3.2 IM 模块

**IM 模块**提供统一的即时消息抽象层，支持多种消息渠道。

#### 3.2.1 message_bus

**功能**：实现线程安全的入站/出站双队列。

**主要组件**：
- `message_bus_init()`：初始化消息总线
- `message_bus_push_inbound()`：推送入站消息
- `message_bus_pop_inbound()`：获取入站消息
- `message_bus_push_outbound()`：推送出站消息
- `message_bus_pop_outbound()`：获取出站消息

#### 3.2.2 通道实现

**功能**：实现各种 IM 渠道的具体连接和消息处理。

**支持的渠道**：
- Telegram
- Discord
- Feishu
- WebSocket
- 串行 CLI

### 3.3 工具模块

**工具模块**提供 MCP 工具的注册和实现。

#### 3.3.1 tools_register

**功能**：初始化和注册所有 MCP 工具。

**主要组件**：
- `tool_registry_init()`：注册工具初始化回调
- `__ai_mcp_init()`：初始化文件系统、cron、心跳、内存、会话、技能，并注册所有工具

#### 3.3.2 工具实现

**支持的工具**：
- `tool_files`：文件操作（读/写/编辑/列表/查找）
- `tool_cron`：调度任务（cron_add/列表/删除）
- `tool_exec`：远程命令执行（仅 Linux）
- `tool_openclaw_ctrl`：OpenClaw 网关控制
- `tool_hw`：硬件外设控制（可选）

### 3.4 内存模块

**内存模块**负责持久化内存和会话管理。

#### 3.4.1 memory_manager

**功能**：管理长期记忆、每日笔记、个性和用户信息。

**主要组件**：
- `memory_read_long_term()`：读取长期记忆
- `memory_read_recent()`：读取最近的每日笔记

#### 3.4.2 session_manager

**功能**：管理会话的 JSONL 持久化。

### 3.5 网关模块

**网关模块**提供网关连接功能。

#### 3.5.1 ws_server

**功能**：设备端 WebSocket 服务器。

#### 3.5.2 acp_client

**功能**：OpenClaw ACP WebSocket 客户端。

### 3.6 Cron 服务

**Cron 服务**提供后台调度任务服务。

**主要组件**：
- `cron_service_init()`：初始化 cron 服务
- `cron_service_start()`：启动 cron 服务

### 3.7 心跳服务

**心跳服务**定期读取 HEARTBEAT.md 来驱动 AI。

**主要组件**：
- `heartbeat_init()`：初始化心跳服务
- `heartbeat_start()`：启动心跳服务

### 3.8 技能加载器

**技能加载器**扫描和安装内置技能 .md 文件。

**主要组件**：
- `skill_loader_init()`：初始化技能加载器
- `skill_loader_build_summary()`：构建技能摘要

## 4. 主要功能

### 4.1 提醒设置

**功能**：用户可以设置基于时间的提醒。

**流程**：
1. 用户发送 "remind me in 5 minutes" 到 Telegram
2. 代理调用 `get_current_time` 获取当前时间
3. 计算目标时间并调用 `cron_add` 添加调度任务
4. 当时间到达时，cron 触发并推送提醒消息

### 4.2 文件管理

**功能**：用户可以读取、写入、编辑和列出文件。

**流程**：
1. 用户发送 "read /memory/MEMORY.md"
2. 代理调用 `read_file` 工具
3. 工具返回文件内容
4. 代理将内容发送给用户

### 4.3 远程执行

**功能**：用户可以在支持的设备上执行命令。

**流程**：
1. 用户发送 "run `ls -la`"（在 Raspberry Pi 上）
2. 代理调用 `tool_exec` 工具
3. 工具执行命令并返回输出
4. 代理将输出发送给用户

### 4.4 内存与日记

**功能**：代理自动将重要信息写入 MEMORY.md 或每日笔记，跨会话保留上下文。

**流程**：
1. 代理分析对话内容
2. 提取重要信息
3. 调用 `write_file` 工具将信息写入相应文件

### 4.5 OpenClaw 控制

**功能**：用户可以发送夹娃娃机命令。

**流程**：
1. 用户发送夹娃娃机命令
2. 代理通过 ACP/WebSocket 网关发送控制指令
3. 网关执行命令并返回结果
4. 代理将结果发送给用户

## 5. 技术栈

| 类别 | 技术/组件 | 用途 |
|------|-----------|------|
| 编程语言 | C | 主要开发语言 |
| 框架 | TuyaOpen C SDK | 硬件抽象、网络、云连接 |
| 存储 | LittleFS / FlashDB | 持久化存储 |
| 通信 | MQTT, WebSocket, HTTP | 设备-云通信 |
| AI | TuyaOpen AI Agent | 云 AI 推理 |
| 工具 | MCP (Model Context Protocol) | 工具调用框架 |
| 构建系统 | CMake, Kconfig | 构建和配置管理 |

## 6. 项目结构

```
DuckyClaw/
├── agent/                  # 核心代理循环和上下文构建器
│   ├── agent_loop.c/h      # 外循环 + 内工具迭代循环（信号量同步）
│   └── context_builder.c/h # 系统提示组装（规则、内存、技能、个性）
│
├── IM/                     # 统一即时消息抽象层
│   ├── bus/message_bus.c/h # 线程安全入站/出站双队列
│   ├── channels/           # Telegram / Discord / Feishu 机器人实现
│   ├── cli/serial_cli.c/h  # 本地串行/CLI 输入通道
│   ├── proxy/http_proxy.c/h# TLS HTTP 客户端（代理模式）
│   ├── certs/              # TLS CA 证书包
│   ├── im_api.h            # 统一 IM 接口声明
│   ├── im_config.h         # IM 常量和配置
│   └── im_utils.c/h        # IM 实用函数
│
├── tools/                  # MCP 工具注册和实现
│   ├── tools_register.c/h  # 统一工具注册入口点
│   ├── tool_files.c/h      # 文件操作（读/写/编辑/列表/查找）
│   ├── tool_cron.c/h       # 调度任务（cron_add/列表/删除）
│   ├── tool_exec.c/h       # 远程命令执行（仅 Linux）
│   └── tool_openclaw_ctrl.c/h # OpenClaw 网关控制
│
├── memory/                 # 持久内存和会话管理
│   ├── memory_manager.c/h  # MEMORY.md, 每日笔记, SOUL.md, USER.md
│   └── session_manager.c/h # 会话 JSONL 持久化
│
├── gateway/                # 网关连接
│   ├── ws_server.c/h       # 设备端 WebSocket 服务器
│   └── acp_client.c/h      # OpenClaw ACP WebSocket 客户端
│
├── cron_service/           # 后台调度任务服务
│   └── cron_service.c/h    # 内存作业表 + cron.json 持久化 + 调度器线程
│
├── heartbeat/              # 心跳服务
│   └── heartbeat.c/h       # 定期读取 HEARTBEAT.md 来驱动 AI
│
├── skills/                 # 技能加载器
│   └── skill_loader.c/h    # 扫描/安装内置技能 .md 文件
│
├── src/                    # 应用程序胶水层
│   ├── tuya_app_main.c     # 入口点 user_main()，初始化编排
│   ├── ducky_claw_chat.c   # AI 流事件处理，到 agent_loop 的信号量桥接
│   ├── app_im.c            # IM 初始化，出站调度，通道切换
│   ├── cli_cmd.c           # 扩展 CLI 命令
│   └── reset_netcfg.c      # 网络配置重置逻辑
│
├── include/                # 全局头文件
│   ├── tuya_app_config.h   # 产品 ID，通道令牌，网关配置默认值
│   ├── tuya_app_config_secrets.h(.example) # 敏感凭证（gitignored）
│   ├── app_im.h            # IM 应用程序接口
│   └── ducky_claw_chat.h   # 聊天模块接口
│
├── ai_components/          # TuyaOpen AI 组件适配器（子 CMake）
├── config/                 # 板级 Kconfig 快照
│   ├── RaspberryPi.config
│   ├── TUYA_T5AI_BOARD_LCD_3.5_CAMERA.config
│   └── ESP32S3_BREAD_COMPACT_WIFI.config
│
├── CMakeLists.txt          # 应用程序级 CMake 构建脚本
├── Kconfig                 # 聚合子模块 Kconfigs
├── CLAUDE.md               # AI 编码助手指南（Claude Code）
├── AGENTS.md               # 项目指南
└── TuyaOpen/               # SDK 子模块（git submodule）
```

## 7. 关键类与函数

### 7.1 Agent 模块

#### agent_loop.c

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `agent_loop_init()` | 初始化代理循环并启动其线程 | 无 | `OPERATE_RET` |
| `agent_loop_task()` | 外循环：等待用户消息，运行内工具循环 | `void *arg` | 无 |
| `__build_and_send()` | 构建完整提示并发送到云端 AI | `const char *content`：当前消息内容<br>`bool is_tool`：是否为工具结果<br>`bool summarize`：是否请求总结 | 无 |
| `__on_tool_executed()` | MCP 工具执行钩子 | `const char *tool_name`：工具名称<br>`OPERATE_RET rt`：返回码<br>`const MCP_RETURN_VALUE_T *ret_val`：返回值<br>`void *user_data`：用户数据 | 无 |
| `build_current_context()` | 将角色/内容对追加到共享历史 | `const char *role`：角色<br>`const char *content`：内容 | `OPERATE_RET` |
| `agent_loop_set_last_response()` | 存储完成的 AI 文本流 | `const char *text`：文本 | 无 |
| `agent_loop_notify_turn_done()` | 通知轮次完成 | 无 | 无 |
| `agent_loop_in_tool_loop()` | 返回内工具循环是否活跃 | 无 | `bool` |

#### context_builder.c

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `context_build_system_prompt()` | 构建系统提示 | `char *buf`：缓冲区<br>`size_t size`：缓冲区大小 | `size_t`：写入的字节数 |
| `append_file()` | 将文件内容追加到缓冲区 | `char *buf`：缓冲区<br>`size_t size`：缓冲区大小<br>`size_t offset`：偏移量<br>`const char *path`：文件路径<br>`const char *header`：标题 | `size_t`：新的偏移量 |

### 7.2 IM 模块

#### message_bus.c

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `message_bus_init()` | 初始化消息总线 | 无 | `OPERATE_RET` |
| `message_bus_push_inbound()` | 推送入站消息 | `const im_msg_t *msg`：消息 | `OPERATE_RET` |
| `message_bus_pop_inbound()` | 获取入站消息 | `im_msg_t *msg`：消息<br>`uint32_t timeout_ms`：超时时间 | `OPERATE_RET` |
| `message_bus_push_outbound()` | 推送出站消息 | `const im_msg_t *msg`：消息 | `OPERATE_RET` |
| `message_bus_pop_outbound()` | 获取出站消息 | `im_msg_t *msg`：消息<br>`uint32_t timeout_ms`：超时时间 | `OPERATE_RET` |

### 7.3 工具模块

#### tools_register.c

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `tool_registry_init()` | 初始化和注册所有 MCP 工具 | 无 | `OPERATE_RET` |
| `__ai_mcp_init()` | 初始化文件系统、cron、心跳、内存、会话、技能，并注册所有工具 | `void *data`：数据 | `OPERATE_RET` |

### 7.4 主入口

#### tuya_app_main.c

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `user_main()` | 应用程序主函数 | 无 | 无 |
| `user_event_handler_on()` | 用户定义的事件处理程序 | `tuya_iot_client_t *client`：客户端<br>`tuya_event_msg_t *event`：事件 | 无 |
| `audio_dp_obj_proc()` | 音频数据点处理 | `dp_obj_recv_t *dpobj`：数据点对象 | `OPERATE_RET` |
| `ai_audio_volume_upload()` | 上传音频音量 | 无 | `OPERATE_RET` |
| `user_network_check()` | 用户定义的网络检查回调 | 无 | `bool` |

## 8. 依赖关系

| 模块 | 依赖 | 用途 |
|------|------|------|
| agent | context_builder, tool_files, im_api, app_im, ai_agent, ai_chat_main, ai_mcp_server | 核心代理逻辑 |
| context_builder | tool_files, memory_manager, skill_loader | 构建系统提示 |
| IM | tal_api, tuya_cloud_types, tuya_error_code | 消息处理和通道管理 |
| tools | tal_api, cron_service, heartbeat, memory_manager, session_manager, skill_loader | 工具注册和实现 |
| memory | tal_api, tool_files | 持久化内存管理 |
| gateway | tal_api | 网关连接 |
| cron_service | tal_api | 调度任务管理 |
| heartbeat | tal_api | 心跳服务 |
| skills | tal_api | 技能加载 |
| src | tuya_cloud_types, cJSON, tal_api, tuya_app_config, app_config_kv, tuya_iot, tuya_iot_dp, netmgr, tkl_output, tal_cli, tuya_authorize, board_com_api, ducky_claw_chat, reset_netcfg, app_im, tools_register, ws_server, acp_client, agent_loop | 应用程序入口和业务逻辑 |

## 9. 运行与部署

### 9.1 支持的平台

| 类别 | 模型/平台 |
|------|-----------|
| **MCU** | Tuya T5AI 模块<br>ESP32s |
| **SoC** | Raspberry Pi 4/5/CM4/CM5<br>Linux ARM SoC（Qualcomm/Rockchip/Allwinner 等） |
| **PC** | Linux Ubuntu |

### 9.2 快速开始

#### 安装

```shell
git clone https://github.com/tuya/DuckyClaw.git
cd DuckyClaw
git submodule update --init
```

#### 开发指南
- Tuya T5 MCU 指南：[DuckyClaw 快速开始 (T5-AI)](https://tuyaopen.ai/docs/duckyclaw/ducky-quick-start-T5AI)
- Raspberry Pi 指南：[DuckyClaw 快速开始 (raspberry pi 5)](https://tuyaopen.ai/docs/duckyclaw/ducky-quick-start-raspberry-pi-5)
- ESP32 MCU 指南：[DuckyClaw 快速开始 (ESP32-S3)](https://tuyaopen.ai/docs/duckyclaw/ducky-quick-start-ESP32S3)

### 9.3 启动序列

```
user_main()
  ├── cJSON_InitHooks()           // PSRAM 或标准堆
  ├── tal_log_init / tal_kv_init / tal_cli_init ...
  ├── tuya_iot_init()             // Tuya IoT 客户端
  ├── netmgr_init()               // 网络管理器
  ├── board_register_hardware()
  ├── ducky_claw_chat_init()      // 注册 AI 流事件回调
  ├── app_im_init()               // 订阅 MQTT 连接事件（延迟 IM 初始化）
  ├── ws_server_start()           // 启动 WebSocket 服务器
  ├── tool_registry_init()        // 订阅 MQTT 连接 → 一次性 MCP 工具链初始化
  ├── acp_client_init()           // 启动 ACP WebSocket 客户端
  ├── agent_loop_init()           // 创建信号量/互斥锁 → 启动 agent_loop 线程
  └── tuya_iot_start() → for(;;) tuya_iot_yield()
```

MQTT 连接后：
- `app_im_init_evt_cb` → 初始化 message_bus → 启动 IM 机器人 → 启动出站调度器线程
- `__ai_mcp_init` → 初始化文件系统、cron、心跳、内存、会话、技能 → 注册所有 MCP 工具

## 10. 开发指南

### 10.1 编码规范

#### 10.1.1 五大编码指南

**指南 1：使用 TuyaOpen 抽象层 — 永远不要直接调用 POSIX/libc**

所有线程、同步、计时器和内存分配必须通过 `tal_*` API（`tal_thread_*`、`tal_mutex_*`、`tal_semaphore_*`、`tal_queue_*`、`tal_malloc`/`tal_free`）。永远不要使用 `pthread_*`、`malloc`/`free`、`sem_*` 等。当启用外部 PSRAM（`ENABLE_EXT_RAM`）时，使用 `claw_malloc`/`claw_free` 包装器，它们会自动在堆和 PSRAM 之间切换。

**指南 2：通过条件编译隔离平台差异**

对所有平台特定代码（WiFi、显示、相机、音频、PSRAM、仅 Linux 执行）使用统一的保护模式 `#if defined(X) && (X == 1)`。添加新功能时，遵循相同的模式并在 Kconfig 中声明相应的 `config` 条目。

**指南 3：所有模块间通信通过消息总线**

任何新的入站消息源（新 IM 通道、传感器事件等）必须构建 `im_msg_t` 并调用 `message_bus_push_inbound()`。`agent_loop_task` 是唯一的消费者。任何通道都不得直接调用 AI 接口或绕过总线。出站消息同样通过 `message_bus_push_outbound()` → `outbound_dispatch_task`。

**指南 4：返回 `OPERATE_RET`，使用 `TUYA_CALL_ERR_*` 宏进行统一错误处理**

所有非回调函数必须返回 `OPERATE_RET`（`OPRT_OK` = 0 表示成功）。对关键步骤使用 `TUYA_CALL_ERR_RETURN`（失败 → 立即返回），对非关键步骤使用 `TUYA_CALL_ERR_LOG`（记录并继续）。内存分配后始终检查 NULL。

**指南 5：遵循 TuyaOpen C 代码风格**

本项目遵循 TuyaOpen C 约定。

#### 10.1.2 文件布局

**.h 文件** 必须遵循以下顺序：
1. 包含保护
2. C++ 保护
3. 包含
4. 宏
5. 类型定义
6. 函数声明

**.c 文件** 必须遵循以下顺序：
1. 包含
2. 宏
3. 类型定义
4. 文件范围变量
5. 前向声明
6. 函数实现

#### 10.1.3 命名约定

| 元素 | 约定 | 示例 |
|------|------|------|
| 文件范围变量 | `s_` 前缀 | `s_ctx`、`s_history_json` |
| 全局变量 | `g_` 前缀 | `g_device_state` |
| 内部/静态函数 | `__` 前缀 | `__on_tool_executed`、`__build_and_send` |
| 结构 typedef | `_T` 后缀 | `MODULE_CTX_T` |
| 枚举 typedef | `_E` 后缀 | `IM_CHANNEL_E` |
| 宏 / 常量 | `UPPER_SNAKE_CASE` | `TOOL_LOOP_MAX`、`STREAM_DATA_MAX_LEN` |
| 公共函数 | `module_action` | `agent_loop_init`、`message_bus_push_inbound` |

### 10.2 插件/技能开发

**技能格式**：技能是 `skills/` 目录中的 `.md` 文件。代理获取它们的摘要并遵循说明。使用以下结构：

```markdown
# 技能标题

一行描述。

## 何时使用
当用户询问 X / 当 Y 发生时。

## 如何使用
1. 调用 tool_A 与 ...
2. 然后调用 tool_B ...
3. 回复 ...

## 示例
用户: "..." → 使用 get_current_time，然后 web_search "..."，然后回复 "..."
```

添加技能：在 skills 目录中放置新的 `name.md`（例如通过 `write_file` 工具），或在 `skills/skill_loader.c` 中添加内置技能。

### 10.3 调试技巧

- 使用 `PR_DEBUG`、`PR_INFO`、`PR_WARN`、`PR_ERR` 进行日志记录
- 使用 `tal_system_get_free_heap_size()` 监控内存使用
- 使用 `tal_mutex_lock`/`tal_mutex_unlock` 确保线程安全
- 使用 `TUYA_CALL_ERR_RETURN` 和 `TUYA_CALL_ERR_LOG` 进行错误处理
- 遵循启动序列确保模块初始化顺序正确

## 11. 总结

DuckyClaw 是一个创新的硬件导向 AI 代理项目，通过统一的消息接口和灵活的工具系统，实现了边缘设备上的自然语言交互。其核心价值在于将 AI 能力与实际硬件操作相结合，为用户提供直观、便捷的设备控制体验。

项目采用模块化设计，具有良好的可扩展性和跨平台兼容性，支持从 MCU 到 PC 的多种设备类型。通过 TuyaOpen C SDK 的强大抽象能力，DuckyClaw 能够轻松集成各种硬件外设和云服务，为开发者提供了一个理想的 AIoT 应用开发平台。

未来，DuckyClaw 可以通过添加更多工具、支持更多消息渠道、增强本地 AI 能力等方式进一步扩展其功能，为用户带来更智能、更个性化的硬件交互体验。
# 女生头像生成功能

## 功能介绍

DuckyClaw支持生成随机女生头像并显示在LCD屏幕上。头像来自精心挑选的2000+张图片库。

## 使用方法

### 语音控制

说出以下任一指令：
- "生成一个女生头像"
- "显示女生头像"
- "来一张女生头像"
- "Generate a girl avatar"
- "Show me a girl avatar"

### 飞书/IM消息

发送消息：
- "生成女生头像"
- "女生头像"
- "girl avatar"

### MCP工具调用

```json
{
  "tool": "device_generate_girl_avatar"
}
```

## 技术细节

### API接口

- **URL**: https://api.xyttkx.cn/avatar.php
- **类型**: 免费公开API
- **图片数量**: 2000+
- **更新**: 人工挑选上传
- **随机性**: 每次请求返回不同头像

### 实现流程

```
用户请求 → MCP工具 → 下载头像 → 转换格式 → 显示到LCD
```

1. 用户发起请求
2. 调用 `device_generate_girl_avatar` 工具
3. 从API下载JPEG图片
4. 通过 `ai_picture_output` 管道处理
5. 转换为RGB565格式
6. 显示到480x480 LCD屏幕

### 性能指标

- **下载时间**: 1-3秒（取决于网络）
- **转换时间**: <1秒
- **总耗时**: 2-5秒
- **内存使用**: ~500KB（PSRAM）

### 相关文件

- `ai_components/ai_mcp/src/ai_mcp_tools.c` - 工具实现
- `ai_components/ai_picture/src/ai_picture_output.c` - 图片下载
- `ai_components/ai_picture/src/ai_picture_convert.c` - 格式转换
- `src/ducky_claw_chat.c` - 显示处理

## 扩展功能

### 添加更多头像API

可以轻松添加其他头像API：

```c
static OPERATE_RET __generate_avatar(const MCP_PROPERTY_LIST_T *properties, 
                                      MCP_RETURN_VALUE_T *ret_val, 
                                      void *user_data)
{
    const char *avatar_url = "https://your-api.com/avatar";
    return ai_picture_output_start(avatar_url);
}
```

### 支持的其他API

可以集成这些免费头像API：
- **RandomUser.me**: https://randomuser.me/api/portraits/women/1.jpg
- **UIFaces**: https://uifaces.co/api
- **Pravatar**: https://i.pravatar.cc/300
- **DiceBear**: https://avatars.dicebear.com/api/female/random.svg

### 添加参数支持

可以扩展工具支持更多参数：

```c
MCP_PROP_STR("style", "Avatar style: anime, realistic, cartoon"),
MCP_PROP_INT_RANGE("size", "Image size in pixels", 100, 1000)
```

## 故障排除

### 问题：图片无法显示

**可能原因：**
1. 网络连接问题
2. API服务暂时不可用
3. 图片格式不支持

**解决方法：**
- 检查WiFi连接
- 稍后重试
- 查看串口日志

### 问题：显示速度慢

**优化建议：**
- 使用更快的网络
- 考虑本地缓存常用头像
- 预加载下一张头像

### 问题：图片质量不佳

**说明：**
- API返回的图片质量由源决定
- 可以切换到其他高质量API
- 或使用本地高清图片库

## 示例场景

### 场景1：随机头像展示

用户："给我看一个女生头像"
→ 系统立即显示随机头像

### 场景2：连续生成

用户："再来一张"
→ 系统生成新的随机头像

### 场景3：集成到应用

可以将此功能集成到：
- 用户注册流程（选择默认头像）
- 聊天应用（随机头像）
- 相册应用（头像墙）

## 隐私说明

- 所有头像来自公开API
- 图片经过人工审核
- 不包含个人隐私信息
- 仅用于展示目的

## 未来计划

- [ ] 支持男生头像
- [ ] 支持动物头像
- [ ] 本地头像库
- [ ] 头像收藏功能
- [ ] 自定义头像上传
- [ ] 头像编辑功能

## 贡献

欢迎提交PR添加更多头像API或改进功能！

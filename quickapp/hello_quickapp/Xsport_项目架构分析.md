# Xsport — 中学生体测手表应用架构分析

## 一、项目概况

| 指标 | 值 |
|---|---|
| 项目名 | Xsport |
| 包名 | com.xsport.fitness |
| 框架 | OpenVela AIOT Toolkit（快应用） |
| 目标设备 | 智能手表（466×466 圆形屏幕） |
| 文件格式 | `.ux`（类 Vue 单文件组件：script + template + style） |
| 源文件数 | 15 个（9 页面 + 6 公共模块） |
| 代码总量 | **4,382 行** |
| 构建工具 | aiot CLI v2.0.5，Node v22 |
| HTTP 通信 | `@system.fetch` + JSON POST |
| 实时通信 | MQTT 3.1.1 over WebSocket（自研轻量客户端） |

---

## 二、目录结构

```
src/
├── app.ux                     ← 应用入口（61行），全局状态 + 离线同步初始化
├── manifest.json              ← 配置清单：路由、权限、设备类型
├── config-watch.json          ← 手表专用配置
├── common/                    ← 公共模块层
│   ├── constants.js           ← 常量：8种运动、评分、颜色、路由、MQTT配置（161行）
│   ├── storage.js             ← 封装 @system.storage（Promise 化）（123行）
│   ├── network.js             ← HTTP 请求封装 + 设备注册 + 记录上传（249行）
│   ├── sync.js                ← 离线队列 + 网络监听 + 自动同步（324行）
│   ├── health.js              ← @service.health 心率/血氧实时采集（231行）
│   ├── mqtt.js                ← 自研 MQTT 3.1.1 客户端（WebSocket）（726行）
│   └── logo.png               ← 应用图标（圆形，橙色渐变 + 白色跑步人形）
├── pages/
│   ├── index/index.ux         ← 登录页（257行）
│   ├── home/home.ux           ← 首页运动列表（115行）
│   ├── sport/sport.ux         ← 运动详情页（330行）
│   ├── test/test.ux           ← 训练中页面（446行）★ 核心页面
│   ├── result/result.ux       ← 训练结果页（312行）
│   ├── history/history.ux     ← 历史记录页（531行）
│   ├── detail/detail.ux       ← 记录详情页（282行）
│   └── settings/settings.ux   ← 设置页（234行）
├── i18n/                      ← 国际化
│   ├── defaults.json
│   ├── en.json
│   └── zh-CN.json
├── build/                     ← 构建输出目录
├── dist/                      ← 发布包目录
└── package.json               ← 项目配置
```

---

## 三、页面链路与导航

### 主流程

```
index(登录) → home(选择运动) → sport(查看详情) → test(训练中) → result(结果/上传)
```

### 辅助流程

```
home → history → detail       （查看历史记录）
home → settings               （配置学号、服务器、开关）
result → test                 （再测一次）
```

### manifest.json 路由配置

```json
{
  "router": {
    "entry": "pages/index",
    "pages": {
      "pages/index":   { "component": "index" },
      "pages/home":    { "component": "home" },
      "pages/sport":   { "component": "sport" },
      "pages/test":    { "component": "test" },
      "pages/result":  { "component": "result" },
      "pages/history": { "component": "history" },
      "pages/detail":  { "component": "detail" },
      "pages/settings":{ "component": "settings" }
    }
  }
}
```

---

## 四、8 种运动 × 3 种训练模式

| 模式 | 运动项目 | test.ux 显示 | 计分方式 |
|---|---|---|---|
| `count` | 跳绳、仰卧起坐、引体向上 | 次数 + 倒计时 | 次数阶梯：140+→90分，110+→80分 |
| `time` | 50m跑、800m跑、1000m跑 | 距离(米) + 用时 + 配速 | 用时线性插值：越快越高 |
| `distance` | 立定跳远、坐位体前屈 | 厘米(cm) + 心率/消耗 | 基于数值 |

### constants.js 运动定义

```javascript
const TEST_TYPES = [
  { id: 'jump_rope',  label: '跳绳',      mode: 'count',    unit: '次',  duration: 60  },
  { id: '50m_run',    label: '50米跑',    mode: 'time',     unit: '米',  duration: null },
  { id: '800m_run',   label: '800米跑',   mode: 'time',     unit: '米',  duration: null },
  { id: '1000m_run',  label: '1000米跑',  mode: 'time',     unit: '米',  duration: null },
  { id: 'long_jump',  label: '立定跳远',  mode: 'distance', unit: '厘米', duration: null },
  { id: 'sit_ups',    label: '仰卧起坐',  mode: 'count',    unit: '次',  duration: 60  },
  { id: 'pull_up',    label: '引体向上',  mode: 'count',    unit: '次',  duration: 60  },
  { id: 'sit_reach',  label: '坐位体前屈',mode: 'distance', unit: '厘米', duration: null }
]
```

### 评分算法

| 模式 | 算法 | 说明 |
|---|---|---|
| **跑步(time)** | 线性插值 | 50m: 6s→100分, 15s→10分；800m: 180s→100分, 360s→10分；1000m: 210s→100分, 400s→10分 |
| **计数(count)** | 阶梯函数 | 140+→90分, 110+→80分, 80+→60分 |
| **测距(distance)** | 基于数值 | 立定跳远和坐位体前屈使用各自基准 |

---

## 五、核心页面详解

### 5.1 登录页 (`index/index.ux` — 257行)

- 表单：学号 + 密码 + 设备ID（可编辑）+ 服务器地址
- 功能：登录验证、密码显隐切换、设备自动注册
- 网络：POST `/api/v1/students/login`，注册 POST `/api/v1/devices/register`
- 跳转：登录成功 → `router.replace('/pages/home')`

### 5.2 首页 (`home/home.ux` — 115行)

- 电池栏（20px）+ 标题 "运动" + 8 种运动卡片列表
- 每行：40×40px 圆形彩色图标 + 运动名称（左对齐）
- 行高 52px，宽 400px，深灰卡片 `#252525`，26px 大圆角
- 点击运动 → `router.push('/pages/sport', { sportId, sportLabel })`

### 5.3 运动详情页 (`sport/sport.ux` — 330行)

- 单页紧凑版：电池栏 + 标题行 + 数据行(今日/玩法/昨日) + 最近记录(最多3条) + 底部统计
- 8 种运动各有差异化默认数据（跳绳 140次、50m跑 7.8秒、1000m跑 3:50等）
- 底部统计：平均心率 / 最高心率 / 评分 / 用时
- "开始训练" 按钮 → `router.push('/pages/test')`

### 5.4 训练中页面 (`test/test.ux` — 446行) ★

**双模式显示架构：**

| 模式 | 跑步模式 (`mode === 'time'`) | 计数模式 (`mode === 'count/distance'`) |
|---|---|---|
| 主数据 | 距离大数字（米） | 计数值 + 单位 |
| 次数据 | 用时 `MM:SS` | 倒计时 `MM:SS` |
| 数据栏 | 配速 `M'SS"` / 心率 / 消耗 | 心率 / 消耗 |
| 自动停止 | 到达目标距离 → `finishTraining()` | 倒计时归零 → `finishTraining()` |

**布局规格（466×466 圆屏适配）：**

| 区域 | 高度 | 宽度 | y 起始 |
|---|---|---|---|
| 电池栏 | 20px | 340px | y=68 |
| 标题行 | 28px | 340px | y=89 |
| 数据区 | ~128px | 400px | y=119 |
| 按钮组 | 54px | 200px | y=257 |

**核心逻辑：**

```javascript
// 跑步模拟：3.5~5 m/s，到达目标自动停止
var metersPerSec = 3.5 + Math.random() * 1.5
self.runDistance = Math.round(self.runDistance + metersPerSec)
if (self.runDistance >= targetDist) {
  self.runDistance = targetDist
  self.finishTraining()  // 自动停止
  return
}

// 配速计算：分钟/公里
var paceMin = (self.runElapsed / 60) / (self.runDistance / 1000)
self.runPace = Math.floor(paceMin) + "'" + String(ps).padStart(2, '0') + '"'
```

### 5.5 结果页 (`result/result.ux` — 312行)

- 电池栏 + "训练完成" + 大数字(成绩) + 等级标签
- 4 格统计：用时 / 平均心率 / 最高心率 / 得分
- 上传状态栏（成功/失败/等待）
- 按钮：查看记录 / 再测一次

### 5.6 历史记录页 (`history/history.ux` — 531行)

- 从 `storage.get('history_records')` 读取本地队列
- 卡片列表展示：运动名、成绩、日期、评分颜色

### 5.7 记录详情页 (`detail/detail.ux` — 282行)

- 展示单条记录的完整信息
- 评分等级、心率数据、上传状态

### 5.8 设置页 (`settings/settings.ux` — 234行)

- 学号输入、服务器地址配置
- MQTT 实时推送开关
- 健康数据采集开关

---

## 六、公共模块详解

### 6.1 constants.js（161行）

常量配置中心，包含：
- `TEST_TYPES`：8 种运动的完整定义
- `SPORT_COLORS`：每种运动的主题色
- `ROUTES`：页面路由常量
- `MQTT_CONFIG`：MQTT 服务器连接配置
- `SCORE_LEVELS`：评分等级定义
- `PUSH_EVENTS`：MQTT 推送事件类型
- `HEART_RATE`：心率阈值配置

### 6.2 storage.js（123行）

封装 `@system.storage`，提供 Promise 化的 API：
- `read(key)` / `write(key, data)` / `remove(key)` / `clear()`
- `readJSON(key)` / `writeJSON(key, data)`：自动序列化/反序列化
- `readBulk(keys)` / `writeBulk(entries)`：批量操作

### 6.3 network.js（249行）

HTTP 请求封装（基于 `@system.fetch`）：
- `request(method, path, data)`：统一请求入口
- `registerDevice()`：设备注册 POST
- `uploadRecords(records)`：批量上传体测记录
- `fetchResults(params)`：查询体测成绩
- `keepAlive()`：心跳保活

### 6.4 sync.js（324行）— 离线队列核心

**离线优先架构：**
```
体测完成 → saveRecord(本地队列, _synced=false)
  ├─ 联网 → markSynced → 移入 cache
  └─ 离线 → 留队 → startNetworkMonitor → 联网自动 syncPending()
```

核心功能：
- `_PENDING_KEY` / `_CACHE_KEY`：本地存储键
- `_MAX_RETRY = 3`：最大重试次数
- `_DEDUP_WINDOW = 30000`：30 秒去重窗口
- `checkDuplicate()`：防重复提交
- `startNetworkMonitor()`：网络状态监听
- `syncPending()`：批量同步待上传记录

### 6.5 health.js（231行）

封装 `@service.health` API：
- `startHeartRate(callback)`：启动实时心率采集（1秒间隔）
- `startSpO2(callback)`：启动血氧采集
- `subscribeHeartRate(callback)`：心率实时订阅
- `stopHeartRate()` / `stopSpO2()`：停止采集
- 自动降级：服务不可用时跳过，不影响主流程

### 6.6 mqtt.js（726行）— 自研轻量 MQTT 客户端

自研 MQTT 3.1.1 协议实现，基于 WebSocket 传输：

**协议包构造：**
- `_buildConnectPacket(clientId, options)`：CONNECT 包
- `_buildPublishPacket(topic, payload, qos, retain)`：PUBLISH 包
- `_buildPingreqPacket()`：PINGREQ 心跳包

**连接管理：**
- `connect(config)`：建立 WebSocket 连接
- `_reconnect()`：指数退避重连（base=2s, max=60s）
- `_sendPing()`：心跳保活（keepAlive/2 间隔）

**消息处理：**
- `subscribe(topic, callback)` / `unsubscribe(topic)`
- `publish(topic, message, options)`
- 离线消息队列持久化（连接断开时缓存，恢复后自动发送）

---

## 七、数据流架构

### 7.1 训练数据流

```
用户操作 test.ux
    │
    ├─ 实时数据 → UI 更新（距离/次数/心率/配速）
    │
    ├─ 训练结束
    │   ├─ _calcScore()          ← 评分计算
    │   ├─ storage.set()         ← 本地暂存 last_result
    │   ├─ _appendHistory()      ← 追加历史队列（最多 20 条）
    │   └─ _httpUpload()         ← HTTP POST 上传服务器
    │       ├─ 跑步: { distance, time_sec, avg_hr, max_hr, score }
    │       └─ 计数: { count, duration_sec, avg_hr, max_hr, score }
    │
    └─ router.push('/pages/result')  ← 跳转结果页
```

### 7.2 离线同步流

```
体测完成
    │
    ├─ 联网状态 → 直接 HTTP POST → 成功标记 synced
    │
    └─ 离线状态
        ├─ saveRecord(本地队列, _synced=false)
        ├─ startNetworkMonitor()
        └─ 网络恢复 → syncPending() → 批量上传 → 标记 synced
```

### 7.3 心率数据流

```
health.js
    ├─ startHeartRate(callback)  ← 1秒采集间隔
    ├─ subscribeHeartRate(cb)    ← 实时订阅
    │
    └─ test.ux 接收
        ├─ heartRate = 实时值
        ├─ _hrSum += hr          ← 累加求平均
        ├─ _hrCount++
        └─ _maxHr = Math.max()   ← 记录最高值
```

---

## 八、技术约束

这是 OpenVela AIOT 平台的特殊限制，开发时必须遵守：

| 约束 | 影响 | 应对方案 |
|---|---|---|
| 466px 圆形屏幕 | y<80 区域严重裁切，有效宽度随 y 增大 | 电池栏下移到 y=68，宽度 340px |
| 无 `flex:1` 兼容 | Flex 布局受限 | 用固定宽度 + `align-items: center` 居中 |
| 无 `position: absolute/fixed` | 无法绝对定位 | 全部使用 flex 布局 |
| `@system.fetch` 的 data 必须是 String | 不能传对象 | 必须 `JSON.stringify()` 后传入 |
| `.ux` 文件三段式 | script + template + style 各自独立 | 状态通过 data 驱动视图 |
| 边距限制 | padding 不能为负值 | margin 替代 padding 调整布局 |
| 无 border 简写 | `border: 1px solid #xxx` 不支持 | 使用 `border-width` + `border-color` |

---

## 九、构建与部署

```bash
# 开发构建
npm run build        # aiot build，输出到 build/ + dist/

# 构建产物
dist/com.xsport.fitness.debug.1.0.0.rpk    # 可安装的 RPK 包

# 依赖
node_modules/@aiot-toolkit/aiotpack         # 构建工具链 v2.0.5
```

构建流程：
1. 编译 `.ux` 文件（Vue SFC → JS + CSS）
2. Webpack 打包
3. 拷贝资源文件
4. 生成 JSC 字节码（可选）
5. 生成 RPK 签名包
6. 输出到 `.temp_Xsport/dist/`

---

## 十、项目代码量统计

| 文件 | 行数 | 职责 |
|---|---|---|
| `common/mqtt.js` | 726 | MQTT 客户端 |
| `pages/history/history.ux` | 531 | 历史记录 |
| `pages/test/test.ux` | 446 | 训练中页面 |
| `pages/sport/sport.ux` | 330 | 运动详情 |
| `common/sync.js` | 324 | 离线同步 |
| `pages/result/result.ux` | 312 | 训练结果 |
| `pages/detail/detail.ux` | 282 | 记录详情 |
| `pages/index/index.ux` | 257 | 登录页 |
| `common/network.js` | 249 | HTTP 请求 |
| `pages/settings/settings.ux` | 234 | 设置页 |
| `common/health.js` | 231 | 健康数据 |
| `common/constants.js` | 161 | 常量配置 |
| `pages/home/home.ux` | 115 | 首页 |
| `common/storage.js` | 123 | 本地存储 |
| `app.ux` | 61 | 应用入口 |
| **总计** | **4,382** | |

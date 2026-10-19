/**
 * Xsport 体测快应用常量配置
 * 中学生体质健康测试项目类型和评分相关常量
 */

// 默认服务器地址（openvela 模拟器访问宿主机）
export const DEFAULT_SERVER_URL = 'http://101.35.231.154:9000'

// API 前缀
export const API_PREFIX = '/api/v1'

// 本地存储键名
export const STORAGE_KEYS = {
  STUDENT_ID: 'student_id',
  DEVICE_ID: 'device_id',
  SERVER_URL: 'server_url',
  TOKEN: 'token',
  RECORDS_QUEUE: 'records_queue',
  RECORDS_CACHE: 'records_cache',
  LAST_SYNC: 'last_sync_time',
  SYNC_STATE: 'sync_state'
}

// 测试项目配置
//  hint  : 选择页副标题（设计稿 #2）
//  badge : 列表图标圆内示意字
export const TEST_TYPES = [
  {
    id: 'jump_rope',
    label: '跳绳',
    icon: '🏃',
    badge: '绳',
    mode: 'count',
    unit: '次',
    duration: 60,
    hint: '60秒计数',
    description: '1分钟跳绳测试'
  },
  {
    id: '50m_run',
    label: '50米跑',
    icon: '💨',
    badge: '50',
    mode: 'time',
    unit: '秒',
    hint: '计时冲刺',
    description: '50米短跑测试'
  },
  {
    id: '800m_run',
    label: '800米跑',
    icon: '🏃‍♀️',
    badge: '800',
    mode: 'time',
    unit: '秒',
    hint: '耐力测试',
    description: '800米中长跑（女生）'
  },
  {
    id: '1000m_run',
    label: '1000米跑',
    icon: '🏃‍♂️',
    badge: '千',
    mode: 'time',
    unit: '秒',
    hint: '耐力测试',
    description: '1000米中长跑（男生）'
  },
  {
    id: 'long_jump',
    label: '立定跳远',
    icon: '🦘',
    badge: '远',
    mode: 'distance',
    unit: '米',
    hint: '距离测量',
    description: '立定跳远测试'
  },
  {
    id: 'sit_ups',
    label: '仰卧起坐',
    icon: '🔄',
    badge: '仰',
    mode: 'count',
    unit: '个',
    duration: 60,
    hint: '60秒计数',
    description: '1分钟仰卧起坐测试'
  },
  {
    id: 'pull_up',
    label: '引体向上',
    icon: '💪',
    badge: '引',
    mode: 'count',
    unit: '个',
    hint: '次数测试',
    description: '引体向上测试'
  },
  {
    id: 'sit_reach',
    label: '坐位体前屈',
    icon: '🧘',
    badge: '屈',
    mode: 'distance',
    unit: '厘米',
    hint: '柔韧测试',
    description: '坐位体前屈柔韧测试'
  }
]

// 成绩等级
export const GRADE_LEVELS = {
  excellent: { label: '优秀', color: '#4CAF50', min: 90 },
  good: { label: '良好', color: '#2196F3', min: 80 },
  pass: { label: '及格', color: '#FF9800', min: 60 },
  fail: { label: '不及格', color: '#F44336', min: 0 }
}

// 测试模式枚举
export const TEST_MODE = {
  COUNT: 'count',
  TIME: 'time',
  DISTANCE: 'distance'
}

// 设备 ID 前缀
export const DEVICE_ID_PREFIX = 'VELA-WATCH'

// 页面路由
export const ROUTES = {
  INDEX: '/pages/index',
  HOME: '/pages/home',
  SPORT: '/pages/sport',
  TEST: '/pages/test',
  RESULT: '/pages/result',
  HISTORY: '/pages/history',
  DETAIL: '/pages/detail',
  REPORT: '/pages/report',
  REPORT_DETAIL: '/pages/report_detail'
}

// 运动图标色系（匹配设计稿）
export const SPORT_COLORS = {
  jump_rope: '#FF6B35',    // 橙红 — 跳绳
  '50m_run': '#00C9A7',     // 青绿 — 50米跑
  '800m_run': '#FFB800',    // 金黄 — 800米跑
  '1000m_run': '#F0883E',   // 橙色 — 1000米跑
  long_jump: '#A371F7',     // 紫色 — 立定跳远
  sit_ups: '#EC4899',       // 粉红 — 仰卧起坐
  pull_up: '#5B9BD5',       // 蓝色 — 引体向上
  sit_reach: '#2EAADB'      // 天蓝 — 坐位体前屈
}

// 心跳采样间隔（毫秒）
export const HR_SAMPLE_INTERVAL = 1000

// 同步相关
export const SYNC_BATCH_SIZE = 20
export const SYNC_RETRY_DELAY = 5000
export const SYNC_MAX_RETRIES = 3

// ===================== MQTT 配置 =====================
// MQTT Broker 地址（手表通过 WebSocket 连接）
export const MQTT_CONFIG = {
  host: '10.0.2.2',        // Broker 地址（openvela 模拟器访问宿主机）
  port: 1883,               // MQTT 端口
  path: '/mqtt',            // WebSocket 路径
  keepalive: 60,           // 心跳间隔(秒)
  topicPrefix: 'fitness'   // 主题前缀，完整格式: fitness/{student_id}/data
}

// MQTT 上传状态枚举
export const MQTT_STATUS = {
  PENDING: 'pending',       // 等待发送
  SENDING: 'sending',       // 发送中
  SENT: 'sent',             // 已发送（等待 ACK）
  SUCCESS: 'success',       // 发布成功
  FAILED: 'failed',         // 发布失败（进入离线队列）
  OFFLINE_QUEUED: 'offline_queued'  // 离线队列中等待重发
}

// ===================== 设计令牌（与 LVGL 端一致） =====================
export const THEME = {
  BG: '#000000',
  CARD: '#1C1C1E',
  CARD2: '#2C2C2E',
  TEXT: '#FFFFFF',
  SUB: '#9A9AA0',
  DIM: '#5A5A60',
  TEAL: '#2DD4BF',       // 主题青色
  GOLD: '#FFD54A',       // 金色（勋章/优秀）
  RED: '#FF3B30',        // 心率红
  ORANGE: '#FF9F0A',
  PROGRESS_BG: '#333338'
}

// 心率区间名称
export const HR_ZONES = ['恢复', '燃脂', '有氧', '极限']

// 按得分返回等级 {label, color}
export function getGradeInfo(score) {
  var s = Math.round(score)
  if (s >= 90) return { label: '优秀', color: THEME.GOLD }
  if (s >= 80) return { label: '良好', color: THEME.TEAL }
  if (s >= 60) return { label: '及格', color: THEME.SUB }
  return { label: '不及格', color: THEME.RED }
}

// 按类型查找项目配置
export function getTestById(id) {
  for (var i = 0; i < TEST_TYPES.length; i++) {
    if (TEST_TYPES[i].id === id) return TEST_TYPES[i]
  }
  return null
}

// 本地记录统一展示值（兼容 test.ux 写入的 {value, unit, mode} 结构）
export function getRecordDisplay(rec) {
  if (!rec) return { value: '--', unit: '' }
  if (rec.mode === 'time') {
    var sec = rec.duration || rec.value || 0
    var m = Math.floor(sec / 60)
    var s = sec % 60
    return { value: m + "'" + String(s).padStart(2, '0') + '"', unit: '' }
  }
  if (rec.mode === 'distance') {
    return { value: String(rec.value != null ? rec.value : (rec.distance_m || 0)), unit: rec.unit || '米' }
  }
  return { value: String(rec.count != null ? rec.count : (rec.value || 0)), unit: rec.unit || '次' }
}

// 记录所属日期分组 key：今天 / 昨天 / MM-DD
export function getRecordDayLabel(iso) {
  var d = new Date(iso)
  var now = new Date()
  var startOfToday = new Date(now.getFullYear(), now.getMonth(), now.getDate()).getTime()
  var t = d.getTime()
  var DAY = 86400000
  if (t >= startOfToday) return '今天'
  if (t >= startOfToday - DAY) return '昨天'
  return (d.getMonth() + 1) + '-' + d.getDate()
}

// 日期短串 MM-DD HH:mm
export function fmtRecordTime(iso) {
  var d = new Date(iso)
  return (d.getMonth() + 1) + '-' + d.getDate() + ' ' +
         String(d.getHours()).padStart(2, '0') + ':' +
         String(d.getMinutes()).padStart(2, '0')
}


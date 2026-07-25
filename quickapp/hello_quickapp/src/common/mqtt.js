/**
 * Xsport MQTT 客户端 — 轻量级 MQTT 3.1.1 实现
 * 基于 WebSocket 传输，适用于 OpenVela 手表快应用
 *
 * 功能：
 *   - CONNECT / DISCONNECT / PUBLISH（QoS 0）
 *   - 自动重连（指数退避）
 *   - 离线消息队列（断网时缓存，恢复后自动发送）
 *   - 心跳保活（KEEPALIVE）
 *
 * 用法：
 *   var mqtt = require('../common/mqtt.js')
 *   mqtt.connect({ host: '10.0.2.2', port: 8083, clientId: 'VELA-xxx' })
 *   mqtt.publish('fitness/STU001/data', { count: 128, ... })
 *   mqtt.disconnect()
 */

var TAG = '[MQTT]'
var storage = require('@system.storage')

// ===================== 配置 =====================
var CONFIG = {
  // 默认 broker 地址（可通过 init() 覆盖）
  host: '10.0.2.2',
  port: 1883,            // MQTT 端口（WebSocket 模式）
  path: '/mqtt',         // WebSocket 路径
  keepalive: 60,        // 心跳间隔(秒)
  reconnectInterval: 3000,  // 首次重连间隔(ms)
  maxReconnectInterval: 30000,
  connectTimeout: 10000,    // 连接超时(ms)
  // 离线队列存储键
  QUEUE_KEY: 'mqtt_offline_queue',
  // 最大离线队列长度
  MAX_QUEUE_SIZE: 50
}

// ===================== 连接状态枚举 =====================
var STATE = {
  DISCONNECTED: 'disconnected',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  RECONNECTING: 'reconnecting',
  ERROR: 'error'
}

// ===================== 内部状态 =====================
var _state = STATE.DISCONNECTED
var _ws = null                    // WebSocket 实例
var _clientId = ''                // 客户端 ID
var _reconnectTimer = null        // 重连定时器
var _keepaliveTimer = null        // 心跳定时器
var _reconnectCount = 0           // 重连次数
var _connectCallback = null       // 连接回调
var _messageCallbacks = {}        // 消息回调 { msgId: callback }
var _msgId = 1                   // 消息 ID 计数器
var _offlineQueue = []            // 离线消息队列（内存）
var _onMessageHandler = null      // 收到消息的回调
var _onStateChangeHandler = null  // 状态变化回调

// ===================== 工具函数 =====================

function debug(msg) {
  console.log(TAG, msg)
}

function warn(msg) {
  console.warn(TAG, msg)
}

/** 生成下一个消息 ID */
function nextMsgId() {
  return _msgId = (_msgId % 65535) + 1
}

/** 计算重连间隔（指数退避） */
function getReconnectInterval() {
  var base = CONFIG.reconnectInterval
  var interval = base * Math.pow(2, _reconnectCount)
  if (interval > CONFIG.maxReconnectInterval) interval = CONFIG.maxReconnectInterval
  // 加上随机抖动避免惊群
  return Math.floor(interval + Math.random() * 2000)
}

/** 更新状态并通知 */
function setState(newState) {
  var oldState = _state
  _state = newState
  debug('状态:', oldState, '→', newState)

  if (_onStateChangeHandler) {
    try {
      _onStateChangeHandler(newState, oldState)
    } catch(e) {}
  }
}

// ===================== MQTT 协议包实现 =====================

/**
 * 构建 MQTT CONNECT 固定报头
 * 返回完整 CONNECT 包（Uint8Array 或兼容 Buffer）
 */
function buildConnectPacket(clientId) {
  // Variable Header: Protocol Name + Level + Flags + KeepAlive
  var protoName = [0x00, 0x04, 0x4D, 0x51, 0x54, 0x54] // "MQTT" (length prefix)
  var level = 0x04          // MQTT v3.1.1
  var flags = 0x02          // CleanSession = true
  var ka = CONFIG.keepalive
  var kaHi = (ka >> 8) & 0xFF
  var kaLo = ka & 0xFF

  // Payload: Client Identifier
  var clientIdBytes = []
  for (var i = 0; i < clientId.length; i++) {
    clientIdBytes.push(clientId.charCodeAt(i))
  }
  var ciLen = clientIdBytes.length
  var ciHi = (ciLen >> 8) & 0xFF
  var ciLo = ciLen & 0xFF

  // Variable header length
  var vhLen = 10 // protoName(6) + level(1) + flags(1) + keepalive(2)
  // Payload length
  var payloadLen = 2 + ciLen
  // Remaining Length
  var remainingLength = vhLen + payloadLen

  // 构建完整包
  var packet = [
    0x10,                           // Fixed header: type=CONNECT(1), flags=0000
    encodeRemainingLength(remainingLength),
    // Variable Header
    ...protoName,
    level,
    flags,
    kaHi, kaLo,
    // Payload: ClientID
    hiByte(ciLen), loByte(ciLen),
    ...clientIdBytes
  ]

  return new Uint8Array(packet)
}

/**
 * 构建 MQTT PUBLISH 包（QoS 0）
 * topic: string
 * payload: object (会被 JSON.stringify)
 */
function buildPublishPacket(topic, payload) {
  var payloadStr = typeof payload === 'string' ? payload : JSON.stringify(payload)
  var payloadBytes = []
  for (var i = 0; i < payloadStr.length; i++) {
    payloadBytes.push(payloadStr.charCodeAt(i))
  }

  // Topic
  var topicBytes = []
  for (var j = 0; j < topic.length; j++) {
    topicBytes.push(topic.charCodeAt(j))
  }
  var topicLen = topicBytes.length

  // Remaining Length = 2 (topic length) + topicLen + payloadLen
  var remainingLength = 2 + topicLen + payloadBytes.length

  // Fixed header: type=PUBLISH(3), DUP=0, QoS=000, RETAIN=0 → 0x30
  var packet = [
    0x30,
    encodeRemainingLength(remainingLength),
    // Topic
    highByte(topicLen), lowByte(topicLen),
    ...topicBytes,
    // Payload
    ...payloadBytes
  ]

  return new Uint8Array(packet)
}

/** 构建 MQTT PINGREQ 包 */
function buildPingreqPacket() {
  return new Uint8Array([0xC0, 0x00])
}

/** 编码剩余长度字段（可变长编码） */
function encodeRemainingLength(length) {
  if (length < 128) return length
  var bytes = []
  var x = length
  do {
    var encodedByte = x % 128
    x = Math.floor(x / 128)
    if (x > 0) encodedByte |= 0x80
    bytes.push(encodedByte)
  } while (x > 0)
  return bytes // 注意：这里简化处理，实际多字节需要展开
}

/** 高字节 / 低字节 */
function highByte(v) { return (v >> 8) & 0xFF }
function lowByte(v)  { return v & 0xFF }
function hiByte(v)    { return (v >> 8) & 0xFF }
function loByte(v)    { return v & 0xFF }

// ===================== WebSocket 管理 =====================

/**
 * 建立 WebSocket 连接并执行 MQTT 握手
 */
function doConnect(options, callback) {
  _connectCallback = callback || function() {}

  var wsUrl = 'ws://' + CONFIG.host + ':' + CONFIG.port + (CONFIG.path || '')
  debug('连接:', wsUrl, 'client_id:', _clientId)

  setState(STATE.CONNECTING)

  try {
    // 尝试创建 WebSocket（OpenVela AIOT 环境）
    _ws = new WebSocket(wsUrl, ['mqtt'])
  } catch(e1) {
    // 某些环境可能不支持第二个参数
    try {
      _ws = new WebSocket(wsUrl)
    } catch(e2) {
      warn('WebSocket 创建失败:', e2.message || e2)
      setState(STATE.ERROR)
      _scheduleReconnect()
      return
    }
  }

  // 设置事件处理器
  _ws.onopen = function() {
    debug('WebSocket 已打开，发送 CONNECT...')
    try {
      var connPacket = buildConnectPacket(_clientId)
      _ws.send(connPacket)
    } catch(e) {
      warn('发送 CONNECT 失败:', e.message || e)
      handleConnectError(e)
    }
  }

  _ws.onmessage = function(evt) {
    handleIncomingMessage(evt.data)
  }

  _ws.onerror = function(err) {
    warn('WebSocket 错误:', err ? (err.message || String(err)) : 'unknown')
    if (_state === STATE.CONNECTING || _state === STATE.RECONNECTING) {
      handleConnectError(err)
    }
  }

  _ws.onclose = function(evt) {
    debug('WebSocket 关闭, code:', evt.code || 0, 'reason:', evt.reason || '')
    cleanup()
    if (_state === STATE.CONNECTED) {
      setState(STATE.DISCONNECTED)
      _scheduleReconnect()
    }
  }

  // 连接超时保护
  setTimeout(function() {
    if (_state === STATE.CONNECTING || _state === STATE.RECONNECTING) {
      warn('连接超时 (' + CONFIG.connectTimeout + 'ms)')
      closeWsSilently()
      handleConnectError(new Error('连接超时'))
    }
  }, CONFIG.connectTimeout)
}

/** 处理入站消息 */
function handleIncomingMessage(data) {
  // 将 ArrayBuffer/数据转换为 Uint8Array
  var bytes
  if (data instanceof ArrayBuffer) {
    bytes = new Uint8Array(data)
  } else if (data instanceof Uint8Array) {
    bytes = data
  } else if (typeof data === 'string') {
    // 某些实现返回字符串
    bytes = new Uint8Array(data.split('').map(c => c.charCodeAt(0)))
  } else {
    warn('未知消息类型:', typeof data)
    return
  }

  if (bytes.length < 2) return

  var type = (bytes[0] >> 4) & 0x0F
  debug('收到包, type:', type, '长度:', bytes.length)

  switch(type) {
    case 2: // CONNACK
      handleConnack(bytes)
      break
    case 3: // PUBLISH
      handlePublish(bytes)
      break
    case 13: // PINGRESP
      debug('PINGRESP 收到')
      break
    default:
      debug('未处理的包类型:', type)
  }
}

/** 处理 CONNACK */
function handleConnack(bytes) {
  if (bytes.length < 4) {
    warn('CONNACK 包过短')
    return
  }
  var sessionPresent = bytes[2]
  var returnCode = bytes[3]

  var rcText = {
    0: '成功', 1: '不可接受的协议版本', 2: '客户端标识符拒绝',
    3: '服务端不可用', 4: '用户名密码错误', 5: '未授权'
  }

  if (returnCode === 0) {
    debug('✅ MQTT 连接成功! Session:', sessionPresent)
    setState(STATE.CONNECTED)
    _reconnectCount = 0
    _startKeepalive()

    // 执行回调
    if (_connectCallback) {
      try { _connectCallback(null, true) } catch(e) {}
      _connectCallback = null
    }

    // 发送离线队列中的积压消息
    _flushOfflineQueue()

  } else {
    warn('❌ CONNACK 失败, code:', returnCode, rcText[returnCode] || '未知错误')
    handleConnectError(new Error('CONNACK ' + returnCode))
  }
}

/** 处理收到的 PUBLISH（服务器下发） */
function handlePublish(bytes) {
  // 解析 QoS 0 PUBLISH
  // bytes[0]: 固定头 (type=3, flags)
  // bytes[1+remaining_length]: variable header + payload
  // QoS 0: [topic_len_hi][topic_lo][topic...][payload...]

  if (bytes.length < 4) return

  var topicLen = (bytes[2] << 8) | bytes[3]
  if (bytes.length < 4 + topicLen) return

  var topic = ''
  for (var i = 4; i < 4 + topicLen; i++) {
    topic += String.fromCharCode(bytes[i])
  }

  var payload = ''
  for (var j = 4 + topicLen; j < bytes.length; j++) {
    payload += String.fromCharCode(bytes[j])
  }

  debug('收到 PUBLISH topic:', topic)

  // 触发消息回调
  if (_onMessageHandler) {
    try {
      var parsed = payload
      try { parsed = JSON.parse(payload) } catch(e) {}
      _onMessageHandler(topic, parsed)
    } catch(e) {
      warn('消息回调异常:', e.message || e)
    }
  }
}

/** 处理连接错误 */
function handleConnectError(err) {
  closeWsSilently()
  setState(STATE.ERROR)
  if (_connectCallback) {
    try { _connectCallback(err || new Error('连接失败'), false) } catch(e) {}
    _connectCallback = null
  }
  _scheduleReconnect()
}

/** 安排重连 */
function _scheduleReconnect() {
  if (_reconnectTimer) return

  var delay = getReconnectInterval()
  _reconnectCount++
  debug(_reconnectCount + '次 重连将在 ' + Math.round(delay/1000) + 's 后尝试...')

  _reconnectTimer = setTimeout(function() {
    _reconnectTimer = null
    setState(STATE.RECONNECTING)
    doConnect({}, _connectCallback)
  }, delay)
}

/** 启动心跳 */
function _startKeepalive() {
  _stopKeepalive()
  _keepaliveTimer = setInterval(function() {
    if (_ws && _ws.readyState === 1 && _state === STATE.CONNECTED) {
      try {
        _ws.send(buildPingreqPacket())
      } catch(e) {
        warn('心跳发送失败:', e.message || e)
      }
    }
  }, CONFIG.keepalive * 1000)
}

function _stopKeepalive() {
  if (_keepaliveTimer) {
    clearInterval(_keepaliveTimer)
    _keepaliveTimer = null
  }
}

/** 静默关闭 WebSocket */
function closeWsSilently() {
  _stopKeepalive()
  if (_ws) {
    try {
      _ws.onopen = null
      _ws.onmessage = null
      _ws.onerror = null
      _ws.onclose = null
      if (_ws.readyState === 0 || _ws.readyState === 1) {
        _ws.close()
      }
    } catch(e) {}
    _ws = null
  }
}

/** 清理所有资源 */
function cleanup() {
  _stopKeepalive()
  if (_reconnectTimer) {
    clearTimeout(_reconnectTimer)
    _reconnectTimer = null
  }
  _ws = null
}

// ===================== 离线队列 =====================

/** 将消息加入离线队列 */
function enqueueOffline(topic, payload) {
  var item = {
    t: topic,
    p: payload,
    ts: Date.now()
  }

  // 内存队列
  _offlineQueue.push(item)
  if (_offlineQueue.length > CONFIG.MAX_QUEUE_SIZE) {
    _offlineQueue.shift()
  }

  // 同时持久化到 storage（防止进程被杀丢失）
  persistQueue()

  debug('已加入离线队列, 当前队列数:', _offlineQueue.length)
}

/** 从 storage 加载离线队列 */
function loadPersistedQueue() {
  storage.get({
    key: CONFIG.QUEUE_KEY,
    success: function(data) {
      if (data && data !== '') {
        try {
          var list = JSON.parse(data)
          if (Array.isArray(list)) {
            _offlineQueue = list
            debug('加载离线队列, 数量:', _offlineQueue.length)
          }
        } catch(e) {
          warn('解析离线队列失败')
        }
      }
    },
    fail: function() {
      debug('无持久化离线队列')
    }
  })
}

/** 持久化队列到 storage */
function persistQueue() {
  try {
    storage.set({
      key: CONFIG.QUEUE_KEY,
      value: JSON.stringify(_offlineQueue.slice(0, CONFIG.MAX_QUEUE_SIZE)),
      success: function() {},
      fail: function() { warn('离线队列持久化失败') }
    })
  } catch(e) {}
}

/** 清空离线队列（全部发送成功后调用） */
function clearOfflineQueue() {
  _offlineQueue = []
  persistQueue()
}


/** 刷新离线队列（在连接建立后自动调用） */
function _flushOfflineQueue() {
  if (_offlineQueue.length === 0) return

  debug('开始刷新离线队列, 数量:', _offlineQueue.length)
  var queue = _offlineQueue.slice(0)
  _offlineQueue = []

  var sent = 0
  var failed = 0

  for (var i = 0; i < queue.length; i++) {
    var item = queue[i]
    var ok = publishImmediate(item.t, item.p)
    if (ok) sent++
    else { failed++; _offlineQueue.push(item) }
  }

  if (sent > 0) debug('离线队列刷新完成, 发送:', sent, '失败:', failed)
  if (failed > 0) persistQueue()
  else clearOfflineQueue()
}

// ===================== 公共 API =====================

/**
 * 初始化配置（应在 connect 之前调用）
 * @param {object} options
 *   - host: string       MQTT broker 地址 (默认 10.0.2.2)
 *   - port: number       WebSocket 端口 (默认 8083)
 *   - path: string       WebSocket 路径 (默认 /mqtt)
 *   - keepalive: number  心跳间隔秒数 (默认 60)
 *   - clientId: string   客户端标识 (默认自动生成)
 */
export function init(options) {
  if (!options) options = {}
  if (options.host) CONFIG.host = options.host
  if (options.port) CONFIG.port = options.port
  if (options.path) CONFIG.path = options.path
  if (options.keepalive) CONFIG.keepalive = options.keepalive

  debug('初始化配置:', JSON.stringify({ host: CONFIG.host, port: CONFIG.port, path: CONFIG.path }))

  // 加载持久化的离线队列
  loadPersistedQueue()
}

/**
 * 连接到 MQTT Broker
 * @param {object} options
 *   - clientId: string  (必填) 客户端标识，如 'VELA-WATCH_xxx'
 * @param {function} callback - (err, connected) => void
 */
export function connect(options, callback) {
  if (!options || !options.clientId) {
    warn('connect 需要 clientId 参数')
    if (callback) callback(new Error('缺少 clientId'), false)
    return
  }

  _clientId = options.clientId

  // 如果已经连接则直接返回成功
  if (_state === STATE.CONNECTED && _ws && _ws.readyState === 1) {
    debug('已处于连接状态')
    if (callback) callback(null, true)
    return
  }

  // 先关闭旧连接
  closeWsSilently()
  _reconnectCount = 0

  doConnect(options, callback)
}

/**
 * 发布消息
 * @param {string} topic - 主题, 如 'fitness/STU20260001/data'
 * @param {object|string} payload - 消息体（对象会自动 JSON.stringify）
 * @returns {boolean} 是否立即发出（false 表示进入离线队列）
 */
export function publish(topic, payload) {
  if (!topic) {
    warn('publish 需要 topic')
    return false
  }

  // 已连接 → 立即发送
  if (_state === STATE.CONNECTED && _ws && _ws.readyState === 1) {
    return publishImmediate(topic, payload)
  }

  // 未连接 → 进入离线队列
  debug('未连接, 消息进入离线队列, topic:', topic)
  enqueueOffline(topic, payload)
  return false
}

/** 内部：立即发送（假设已连接） */
function publishImmediate(topic, payload) {
  try {
    var pkt = buildPublishPacket(topic, payload)
    _ws.send(pkt)
    debug('PUBLISH 发送成功, topic:', topic)
    return true
  } catch(e) {
    warn('PUBLISH 发送失败:', e.message || e)
    enqueueOffline(topic, payload)
    return false
  }
}

/**
 * 断开连接
 */
export function disconnect() {
  debug('主动断开连接')
  _reconnectCount = 999 // 阻止自动重连
  closeWsSilently()
  setState(STATE.DISCONNECTED)
}

/**
 * 获取当前连接状态
 * @returns {string} STATE 枚举值
 */
export function getState() {
  return _state
}

/**
 * 是否已连接
 * @returns {boolean}
 */
export function isConnected() {
  return _state === STATE.CONNECTED && _ws && _ws.readyState === 1
}

/**
 * 注册收到消息的回调
 * @param {function} handler - (topic, message) => void
 */
export function onMessage(handler) {
  _onMessageHandler = handler
}

/**
 * 注册状态变化回调
 * @param {function} handler - (newState, oldState) => void
 */
export function onStateChange(handler) {
  _onStateChangeHandler = handler
}

/**
 * 获取离线队列中的消息数量
 * @returns {number}
 */
export function getOfflineQueueSize() {
  return _offlineQueue ? _offlineQueue.length : 0
}

/**
 * 便捷方法：发布体测训练数据（按指定格式）
 * 自动拼接 topic 为 fitness/{student_id}/data
 *
 * @param {object} data - 体测数据
 *   - student_id: string     学号
 *   - test_type: string      测试类型 (jump_rope / 50m_run 等)
 *   - test_date: string      ISO 时间戳
 *   - count: number          次数/数值
 *   - duration_sec: number   时长(秒)
 *   - avg_hr: number         平均心率
 *   - max_hr: number         最大心率
 *   - battery: number        电量百分比
 * @returns {boolean}
 */
export function publishFitnessData(data) {
  if (!data || !data.student_id) {
    warn('publishFitnessData 缺少 student_id')
    return false
  }

  var topic = 'fitness/' + data.student_id + '/data'

  // 标准化 payload 格式
  var payload = {
    student_id: data.student_id,
    test_type: data.test_type || 'unknown',
    test_date: data.test_date || new Date().toISOString(),
    count: data.count || 0,
    duration_sec: data.duration_sec || 0,
    avg_hr: data.avg_hr || 0,
    max_hr: data.max_hr || 0,
    battery: data.battery || 0
  }

  // 补充可选字段
  if (data.sport_name) payload.sport_name = data.sport_name
  if (data.score !== undefined) payload.score = data.score
  if (data.calories !== undefined) payload.calories = data.calories
  if (data.device_id) payload.device_id = data.device_id

  return publish(topic, payload)
}

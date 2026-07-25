/**
 * 健康服务模块 - 基于官方 service.health API
 *
 * 官方文档 API:
 *   health.subscribeSample({ dataType, callback, fail })
 *   health.unsubscribeSample({ dataType })
 *   health.DATA_TYPES.HEART_RATE / SPO2 / ...
 *
 * 回调格式: sample = { timeStamp: <毫秒>, value: <数值> }
 *
 * 前置要求 (manifest.json):
 *   features: [{ "name": "service.health" }]
 *   permissions: [{ "name": "hapjs.permission.HEALTH" }]
 *   config.background.features: ["service.health"]
 */

// 尝试加载健康服务，不可用时降级
let health = null
let healthAvailable = false

try {
  health = require('@service.health')
  if (health && typeof health.subscribeSample === 'function') {
    healthAvailable = true
    console.log('[Health] @service.health 加载成功, API: subscribeSample')
  } else if (health) {
    console.warn('[Health] @service.health 加载成功但缺少 subscribeSample, 可用方法:', Object.keys(health))
  } else {
    console.warn('[Health] @service.health 加载结果为 null')
  }
} catch (e) {
  console.warn('[Health] @service.health 不可用:', e.message || e)
}

// ---- 回调存储 ----
let heartRateCallback = null
let spo2Callback = null
let isSubscribedHeartRate = false
let isSubscribedSpo2 = false

/**
 * 订阅心率 (使用官方 subscribeSample API)
 *
 * @param {function} onData  - 心率数据回调 (heartRate: number) => void
 * @param {function} onError - 错误回调 (errMsg: string) => void
 * @param {number}   interval - 忽略（官方 API 不支持自定义间隔）
 * @returns {Promise<boolean>}
 */
export function subscribeHeartRate(onData, onError, interval) {
  return new Promise((resolve) => {
    if (!healthAvailable || !health) {
      console.warn('[Health] 健康服务不可用，跳过心率订阅')
      if (typeof onError === 'function') onError('健康服务不可用')
      resolve(false)
      return
    }

    // 检查 DATA_TYPES
    const dataType = (health.DATA_TYPES && health.DATA_TYPES.HEART_RATE)
      ? health.DATA_TYPES.HEART_RATE
      : 'HEART_RATE'

    console.log('[Health] 开始订阅心率, dataType:', dataType)

    health.subscribeSample({
      dataType: dataType,
      callback: (sample) => {
        // sample = { timeStamp: <毫秒>, value: <bpm> }
        if (sample && typeof sample.value === 'number') {
          heartRateCallback = onData || function () {}
          // 兼容旧接口：只传数值
          heartRateCallback(sample.value)
        }
      },
      fail: (data, code) => {
        console.error('[Health] 订阅心率失败, code:', code, 'data:', data)
        if (typeof onError === 'function') onError(String(code || '订阅失败'))
        resolve(false)
      }
    })

    isSubscribedHeartRate = true
    console.log('[Health] 心率订阅请求已发送')
    resolve(true)
  })
}

/**
 * 取消订阅心率
 * @returns {Promise<boolean>}
 */
export function unsubscribeHeartRate() {
  return new Promise((resolve) => {
    heartRateCallback = null

    if (!isSubscribedHeartRate || !healthAvailable || !health) {
      isSubscribedHeartRate = false
      resolve(true)
      return
    }

    const dataType = (health.DATA_TYPES && health.DATA_TYPES.HEART_RATE)
      ? health.DATA_TYPES.HEART_RATE
      : 'HEART_RATE'

    health.unsubscribeSample({
      dataType: dataType
    })

    isSubscribedHeartRate = false
    console.log('[Health] 已取消心率订阅')
    resolve(true)
  })
}

/**
 * 订阅血氧
 * @param {function} callback - (spo2: number) => void
 * @returns {Promise<boolean>}
 */
export function subscribeSpO2(callback) {
  return new Promise((resolve) => {
    if (!healthAvailable || !health) {
      console.warn('[Health] 健康服务不可用，跳过血氧订阅')
      resolve(false)
      return
    }

    const dataType = (health.DATA_TYPES && health.DATA_TYPES.SPO2)
      ? health.DATA_TYPES.SPO2
      : 'SPO2'

    console.log('[Health] 开始订阅血氧, dataType:', dataType)

    health.subscribeSample({
      dataType: dataType,
      callback: (sample) => {
        if (sample && typeof sample.value === 'number') {
          spo2Callback = callback || function () {}
          spo2Callback(sample.value)
        }
      },
      fail: (data, code) => {
        console.error('[Health] 订阅血氧失败, code:', code)
        resolve(false)
      }
    })

    isSubscribedSpo2 = true
    console.log('[Health] 血氧订阅请求已发送')
    resolve(true)
  })
}

/**
 * 取消订阅血氧
 * @returns {Promise<boolean>}
 */
export function unsubscribeSpO2() {
  return new Promise((resolve) => {
    spo2Callback = null

    if (!isSubscribedSpo2 || !healthAvailable || !health) {
      isSubscribedSpo2 = false
      resolve(true)
      return
    }

    const dataType = (health.DATA_TYPES && health.DATA_TYPES.SPO2)
      ? health.DATA_TYPES.SPO2
      : 'SPO2'

    health.unsubscribeSample({
      dataType: dataType
    })

    isSubscribedSpo2 = false
    console.log('[Health] 已取消血氧订阅')
    resolve(true)
  })
}

/**
 * 获取最近的一条健康数据
 * @param {string} type - 数据类型
 * @returns {Promise<object|null>} { value, timeStamp } 或 { value, time }
 */
export function getRecent(type) {
  return new Promise((resolve) => {
    if (!healthAvailable || !health) {
      resolve(null)
      return
    }

    const dataType = type ||
      ((health.DATA_TYPES && health.DATA_TYPES.HEART_RATE) ? health.DATA_TYPES.HEART_RATE : 'HEART_RATE')

    // 尝试 getRecent，不存在则返回 null
    if (typeof health.getRecent === 'function') {
      health.getRecent({
        dataType: dataType,
        success: (data) => {
          resolve(data)
        },
        fail: () => {
          resolve(null)
        }
      })
    } else {
      resolve(null)
    }
  })
}

/**
 * 清除所有健康数据订阅
 */
export async function clearAllSubscriptions() {
  await unsubscribeHeartRate()
  await unsubscribeSpO2()
}

/**
 * 健康数据类型常量（兼容旧代码引用）
 */
export const HEALTH_TYPE = {
  HEART_RATE: 'HEART_RATE',
  SPO2: 'SPO2',
  STRESS: 'STRESS',
  HEART_RATE_ECG: 'HEART_RATE_ECG'
}

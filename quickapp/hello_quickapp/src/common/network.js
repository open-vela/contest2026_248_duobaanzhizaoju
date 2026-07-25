/**
 * 网络请求模块
 * 封装与体测服务器的 HTTP 通信
 */

import { API_PREFIX, STORAGE_KEYS } from './constants'
import { read, write } from './storage'

let baseUrl = ''

// ---------- 调试工具 ----------
const DEBUG_TAG = '[Xsport-Net]'

function debug(msg, ...args) {
  console.log(`${DEBUG_TAG} ${msg}`, ...args)
}

function debugError(msg, ...args) {
  console.error(`${DEBUG_TAG} ${msg}`, ...args)
}

/**
 * 初始化网络模块
 * @param {string} url - 服务器地址
 */
export function init(url) {
  if (url) {
    baseUrl = url.replace(/\/+$/, '')
    debug('网络模块初始化, baseUrl =', baseUrl)
    write(STORAGE_KEYS.SERVER_URL, baseUrl)
  }
}

/**
 * 获取服务器地址
 * @returns {Promise<string>}
 */
async function getBaseUrl() {
  if (baseUrl) return baseUrl
  const saved = await read(STORAGE_KEYS.SERVER_URL)
  if (saved) {
    baseUrl = saved
    return baseUrl
  }
  baseUrl = 'http://101.35.231.154:9000'
  return baseUrl
}

/**
 * 发送 HTTP 请求（基于 @system.fetch）
 * @param {string} path - API 路径（不含前缀）
 * @param {object} options - 请求选项
 * @returns {Promise<object>}
 */
export async function request(path, options = {}) {
  const base = await getBaseUrl()
  const { method = 'GET', body, headers = {}, token = null } = options

  const fullUrl = `${base}${API_PREFIX}${path}`

  const reqHeaders = {
    'Content-Type': 'application/json',
    ...headers
  }

  // 自动注入 token
  const savedToken = token || await read(STORAGE_KEYS.TOKEN)
  if (savedToken) {
    reqHeaders['Authorization'] = `Bearer ${savedToken}`
  }

  // ========== 详细日志：请求参数 ==========
  debug('========== 请求开始 ==========')
  debug(`URL:   ${fullUrl}`)
  debug(`Method: ${method}`)
  debug(`Headers:`, JSON.stringify(reqHeaders))

  // ========== 关键修复：@system.fetch 的 data 必须是 String ==========
  const NativeFetch = require('@system.fetch')
  
  return new Promise((resolve, reject) => {
    const fetchParams = {
      url: fullUrl,
      method: method,
      header: reqHeaders,
      responseType: 'text',
      success(resp) {
        debug('========== 请求成功 ==========')
        debug(`HTTP Code: ${resp.code}`)
        debug(`响应头:`, JSON.stringify(resp.headers || {}))
        debug(`响应体(前500字):`, (resp.data || '').slice(0, 500))

        const code = resp.code || 200
        try {
          const data = JSON.parse(resp.data)
          debug(`解析后 JSON:`, JSON.stringify(data).slice(0, 500))
          if (code >= 200 && code < 300) {
            resolve(data)
          } else {
            const errMsg = data.detail || `HTTP ${code}`
            debugError(`服务器返回错误: ${errMsg}`)
            reject(new Error(errMsg))
          }
        } catch (e) {
          if (code >= 200 && code < 300) {
            resolve(resp.data)
          } else {
            const errMsg = `HTTP ${code}: ${(resp.data || '').slice(0, 100)}`
            debugError(`响应解析失败: ${errMsg}`)
            reject(new Error(errMsg))
          }
        }
      },
      fail(err, code) {
        debugError('========== 请求失败 ==========')
        debugError(`错误码: ${code}`)
        debugError(`错误详情:`, JSON.stringify(err || {}))
        reject(new Error(`网络错误 (${code}): ${JSON.stringify(err || '')}`))
      }
    }

    // body 处理：JSON.stringify 为字符串（快应用 @system.fetch 要求 data 为 String）
    if (body && method !== 'GET') {
      const dataStr = JSON.stringify(body)
      fetchParams.data = dataStr
      debug(`请求体(JSON): ${dataStr}`)
    } else {
      debug('无请求体')
    }

    debug('fetchParams keys:', Object.keys(fetchParams))
    NativeFetch.fetch(fetchParams)
  })
}

// ---------- API 封装 ----------

/**
 * 设备注册
 * @param {string} deviceId - 设备编号
 * @param {string} studentId - 学生学号（可选）
 * @returns {Promise<object>}
 */
export function registerDevice(deviceId, studentId = null) {
  if (!deviceId) {
    console.error(`${DEBUG_TAG} registerDevice 失败: deviceId 为空`)
    return Promise.reject(new Error('deviceId 不能为空'))
  }
  
  debug('registerDevice 调用, deviceId =', deviceId, 'studentId =', studentId)
  
  const body = {
    device_id: deviceId,
    device_name: 'OpenVela手表',
    student_id: studentId
  }
  debug('registerDevice body:', JSON.stringify(body))
  
  return request('/devices/register', {
    method: 'POST',
    body: body
  })
}

/**
 * 上传体测记录（单条）
 * @param {string} deviceId - 设备编号
 * @param {object} record - 体测记录
 * @returns {Promise<object>}
 */
export async function uploadRecord(deviceId, record) {
  debug('uploadRecord, deviceId =', deviceId, 'record =', JSON.stringify(record).slice(0, 200))
  return request('/fitness/upload/batch', {
    method: 'POST',
    body: {
      device_id: deviceId,
      records: [record]
    }
  })
}

/**
 * 批量上传体测记录
 * @param {string} deviceId - 设备编号
 * @param {Array} records - 体测记录数组
 * @returns {Promise<object>}
 */
export async function uploadRecords(deviceId, records) {
  debug('uploadRecords, deviceId =', deviceId, 'count =', records.length)
  return request('/fitness/upload/batch', {
    method: 'POST',
    body: {
      device_id: deviceId,
      records
    }
  })
}

/**
 * 手动录入体测记录（通过 API 直接写入）
 * @param {object} record - 体测记录
 * @returns {Promise<object>}
 */
export function submitManualRecord(record) {
  return request('/fitness/records/manual', {
    method: 'POST',
    body: record
  })
}

/**
 * 获取学生的体测记录
 * @param {string} studentId - 学生学号
 * @returns {Promise<object>}
 */
export function getStudentRecords(studentId) {
  return request(`/fitness/records/${studentId}`)
}

/**
 * 获取学生的成绩趋势
 * @param {string} studentId - 学生学号
 * @returns {Promise<object>}
 */
export function getStudentTrend(studentId) {
  return request(`/statistics/trend/${studentId}`)
}

/**
 * 获取设备列表
 * @returns {Promise<object>}
 */
export function getDevices() {
  return request('/devices')
}

/**
 * 心跳保活 - 更新设备在线状态
 * @param {string} deviceId - 设备编号
 * @returns {Promise<object>}
 */
export function deviceHeartbeat(deviceId) {
  return request(`/devices/${deviceId}/heartbeat`, {
    method: 'POST',
    body: {}
  }).catch(() => {
    // 心跳失败忽略，不影响主流程
  })
}

// ---------- AI 报告（手表端，无需 token） ----------

/**
 * 获取手表端 AI 周报列表（最近 N 份）
 * @param {string} studentId - 学号
 * @param {number} limit - 返回条数
 * @returns {Promise<Array>}
 */
export function getWatchReports(studentId, limit = 10) {
  debug('getWatchReports, studentId =', studentId, 'limit =', limit)
  return request(`/ai/watch/reports?student_id=${encodeURIComponent(studentId)}&limit=${limit}`)
}

/**
 * 获取手表端单份 AI 报告详情（含完整 HTML）
 * @param {number} reportId - 报告 ID
 * @returns {Promise<object>}
 */
export function getWatchReportDetail(reportId) {
  debug('getWatchReportDetail, reportId =', reportId)
  return request(`/ai/watch/report/${reportId}`)
}

/**
 * 获取手表端最新一份 AI 报告
 * @param {string} studentId - 学号
 * @returns {Promise<object>}
 */
export function getWatchLatestReport(studentId) {
  debug('getWatchLatestReport, studentId =', studentId)
  return request(`/ai/watch/report/latest?student_id=${encodeURIComponent(studentId)}`)
}

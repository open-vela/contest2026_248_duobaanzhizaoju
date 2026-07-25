/**
 * 本地存储模块
 * 封装 OpenVela 快应用的 storage 服务，提供读写本地数据能力
 */

import storage from '@system.storage'

/**
 * 读取存储值
 * @param {string} key - 键名
 * @returns {Promise<*>} 解析后的值，若无则返回 null
 */
export function read(key) {
  return new Promise((resolve) => {
    try {
      storage.get({
        key,
        success: (data) => {
          console.log(`[Storage] 读取成功 [${key}]:`, data)
          resolve(data)
        },
        fail: (err, code) => {
          console.error(`[Storage] 读取失败 [${key}]:`, err, code)
          resolve(null)
        }
      })
    } catch (e) {
      console.error(`[Storage] 读取异常 [${key}]:`, e.message)
      resolve(null)
    }
  })
}

/**
 * 写入存储值
 * @param {string} key - 键名
 * @param {*} value - 要存储的值
 * @returns {Promise<boolean>}
 */
export function write(key, value) {
  return new Promise((resolve) => {
    try {
      storage.set({
        key,
        value,
        success: () => {
          console.log(`[Storage] 写入成功 [${key}]:`, value)
          resolve(true)
        },
        fail: (err, code) => {
          console.error(`[Storage] 写入失败 [${key}]:`, err, code)
          resolve(false)
        }
      })
    } catch (e) {
      console.error(`[Storage] 写入异常 [${key}]:`, e.message)
      resolve(false)
    }
  })
}

/**
 * 删除存储值
 * @param {string} key - 键名
 * @returns {Promise<boolean>}
 */
export function remove(key) {
  return new Promise((resolve) => {
    try {
      storage.delete({
        key,
        success: () => resolve(true),
        fail: () => resolve(false)
      })
    } catch (e) {
      resolve(false)
    }
  })
}

/**
 * 清除所有存储
 * @returns {Promise<boolean>}
 */
export function clear() {
  return new Promise((resolve) => {
    try {
      storage.clear({
        success: () => resolve(true),
        fail: () => resolve(false)
      })
    } catch (e) {
      resolve(false)
    }
  })
}

/**
 * 读取JSON对象
 * @param {string} key
 * @returns {Promise<object|null>}
 */
export async function readJSON(key) {
  const val = await read(key)
  if (val && typeof val === 'string') {
    try {
      return JSON.parse(val)
    } catch (e) {
      return null
    }
  }
  return val || null
}

/**
 * 写入JSON对象
 * @param {string} key
 * @param {object} value
 * @returns {Promise<boolean>}
 */
export function writeJSON(key, value) {
  return write(key, JSON.stringify(value))
}

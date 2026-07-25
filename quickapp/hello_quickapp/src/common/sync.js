/**
 * 同步队列模块
 *
 * 核心机制：本地优先 -> 联网自动推送
 *
 * 队列模型：
 *   records_queue: [
 *     { ...record, _id: string, _synced: boolean, _createdAt: ms, _retries: number }
 *   ]
 *
 * 流程：
 *   体测完成 -> saveRecord(本地保存 _synced=false) -> 尝试立即推送
 *   ├─ 联网成功 -> markSynced -> 移入 records_cache
 *   └─ 离线/失败 -> 留队 -> 监听网络 -> 联网自动 syncPending()
 */

import { STORAGE_KEYS, SYNC_BATCH_SIZE, SYNC_MAX_RETRIES } from './constants'
import { readJSON, writeJSON, write, read } from './storage'
import { uploadRecords, uploadRecord } from './network'

// ---- 内部工具 ----

function uid() {
  return `r_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`
}

/**
 * 读取完整队列
 */
async function readQueue() {
  return (await readJSON(STORAGE_KEYS.RECORDS_QUEUE)) || []
}

/**
 * 覆写队列
 */
async function writeQueue(queue) {
  return writeJSON(STORAGE_KEYS.RECORDS_QUEUE, queue)
}

/**
 * 读取已同步存档
 */
async function readCache() {
  return (await readJSON(STORAGE_KEYS.RECORDS_CACHE)) || []
}

/**
 * 更新状态快照
 */
async function updateSyncState() {
  const queue = await readQueue()
  const pending = queue.filter((r) => !r._synced).length
  await writeJSON(STORAGE_KEYS.SYNC_STATE, {
    pending,
    total: queue.length,
    lastCheck: Date.now()
  })
}

// ---- 对外 API ----

/**
 * 保存一条体测记录到本地队列
 * @param {object} record - 原始体测数据
 * @returns {Promise<object>} 含 _id 的记录
 */
export async function saveRecord(record) {
  const queue = await readQueue()

  // 去重：30 秒内同类型同成绩的记录视为重复
  const now = Date.now()
  const dup = queue.find((r) => {
    return r.test_type === record.test_type &&
           Math.abs(r._createdAt - now) < 30000 &&
           r._synced === false
  })
  if (dup) return dup

  const item = {
    ...record,
    _id: uid(),
    _synced: false,
    _createdAt: now,
    _retries: 0,
    test_date: record.test_date || new Date().toISOString()
  }

  queue.push(item)
  await writeQueue(queue)
  await updateSyncState()

  console.log(`[Sync] 已缓存: ${item._id} (${item.test_type})`)
  return item
}

/**
 * 获取待同步记录
 * @returns {Promise<Array>}
 */
export async function getPendingRecords() {
  const queue = await readQueue()
  return queue.filter((r) => !r._synced)
}

/**
 * 获取所有本地记录（含已同步和未同步）
 * @returns {Promise<Array>}
 */
export async function getAllRecords() {
  const queue = await readQueue()
  return [...queue].sort((a, b) => b._createdAt - a._createdAt)
}

/**
 * 获取已同步记录
 * @returns {Promise<Array>}
 */
export async function getSyncedRecords() {
  const queue = await readQueue()
  return queue.filter((r) => r._synced)
}

/**
 * 获取同步状态快照
 * @returns {Promise<object>} { pending: number, total: number, lastCheck: ms }
 */
export async function getSyncState() {
  return (await readJSON(STORAGE_KEYS.SYNC_STATE)) || { pending: 0, total: 0, lastCheck: 0 }
}

/**
 * 标记指定记录为已同步
 * @param {string[]} ids - 记录 _id 列表
 */
export async function markSynced(ids) {
  const queue = await readQueue()
  const idSet = new Set(ids)
  const syncedAt = Date.now()

  let changed = false
  queue.forEach((r) => {
    if (idSet.has(r._id) && !r._synced) {
      r._synced = true
      r._syncedAt = syncedAt
      changed = true
    }
  })

  if (!changed) return

  // 将已同步记录移到 cache
  const synced = queue.filter((r) => r._synced)
  const unsynced = queue.filter((r) => !r._synced)

  if (synced.length > 0) {
    const cache = await readCache()
    const cleanRecords = synced.map(({ _id, _synced, _createdAt, _retries, ...record }) => ({
      ...record,
      _syncedAt: syncedAt
    }))
    cache.push(...cleanRecords)

    // cache 保留最近 200 条
    const trimmed = cache.slice(-200)
    await writeJSON(STORAGE_KEYS.RECORDS_CACHE, trimmed)
  }

  await writeQueue(unsynced)
  await updateSyncState()

  console.log(`[Sync] 标记已完成: ${ids.length} 条`)
}

/**
 * 核心：推送所有待同步记录到服务器
 * @returns {Promise<{ success: number, failed: number, total: number }>}
 */
export async function syncPending() {
  const queue = await readQueue()
  const pending = queue.filter((r) => !r._synced)

  if (pending.length === 0) {
    console.log('[Sync] 队列为空，跳过')
    return { success: 0, failed: 0, total: 0 }
  }

  console.log(`[Sync] 开始推送 ${pending.length} 条待同步记录`)

  const deviceId = await read(STORAGE_KEYS.DEVICE_ID) || 'unknown'

  let successIds = []
  let failedIds = []
  const totalBatches = Math.ceil(pending.length / SYNC_BATCH_SIZE)

  for (let i = 0; i < pending.length; i += SYNC_BATCH_SIZE) {
    const batch = pending.slice(i, i + SYNC_BATCH_SIZE)
    const batchNum = Math.floor(i / SYNC_BATCH_SIZE) + 1

    try {
      const cleanBatch = batch.map(({ _id, _synced, _createdAt, _retries, ...record }) => record)

      if (cleanBatch.length === 1) {
        await uploadRecord(deviceId, cleanBatch[0])
      } else {
        await uploadRecords(deviceId, cleanBatch)
      }

      successIds.push(...batch.map((r) => r._id))
      console.log(`[Sync] 批次 ${batchNum}/${totalBatches} 成功 (${batch.length} 条)`)
    } catch (error) {
      console.error(`[Sync] 批次 ${batchNum}/${totalBatches} 失败:`, error.message)

      // 重试次数 + 1
      const queue2 = await readQueue()
      batch.forEach((r) => {
        const item = queue2.find((q) => q._id === r._id)
        if (item && item._retries < SYNC_MAX_RETRIES) {
          item._retries += 1
          failedIds.push(r._id)
        } else if (item && item._retries >= SYNC_MAX_RETRIES) {
          item._synced = true
          item._error = error.message
          console.warn(`[Sync] ${r._id} 重试 ${SYNC_MAX_RETRIES} 次后放弃`)
        }
      })
      await writeQueue(queue2)
    }
  }

  if (successIds.length > 0) {
    await markSynced(successIds)
  }

  await write(STORAGE_KEYS.LAST_SYNC, String(Date.now()))
  await updateSyncState()

  const result = {
    success: successIds.length,
    failed: failedIds.length,
    total: pending.length
  }
  console.log(`[Sync] 完成: 成功 ${result.success}, 失败 ${result.failed}, 共 ${result.total}`)
  return result
}

/**
 * 删除已完成（已同步）的队列记录，释放空间
 */
export async function cleanupSynced() {
  const queue = await readQueue()
  const unsynced = queue.filter((r) => !r._synced)
  await writeQueue(unsynced)
  await updateSyncState()
}

// ---- 网络状态监听 ----

let networkSubscribed = false

/**
 * 启动网络状态监听
 * 当网络恢复时自动触发同步
 */
export function startNetworkMonitor() {
  if (networkSubscribed) return

  try {
    const network = require('@system.network')

    network.subscribe({
      success: () => {
        console.log('[Sync] 网络监听已启动')
        networkSubscribed = true
      },
      fail: (err) => {
        console.warn('[Sync] 网络监听启动失败:', err)
      },
      callback: async (data) => {
        console.log(`[Sync] 网络状态变化: ${data.type}`)

        if (data.type === 'wifi' || data.type === 'cellular') {
          const state = await getSyncState()
          if (state.pending > 0) {
            console.log(`[Sync] 网络恢复，自动推送 ${state.pending} 条`)
            try {
              await syncPending()
            } catch (e) {
              console.error('[Sync] 自动推送失败:', e.message)
            }
          }
        }
      }
    })
  } catch (e) {
    console.warn('[Sync] 网络服务不可用:', e.message)
  }
}

/**
 * 手动检查网络并同步（用于应用启动时）
 */
export async function checkAndSync() {
  try {
    const network = require('@system.network')

    network.getType({
      success: async (data) => {
        if (data.type === 'wifi' || data.type === 'cellular') {
          const state = await getSyncState()
          if (state.pending > 0) {
            console.log(`[Sync] 启动时检测到 ${state.pending} 条待同步，开始推送`)
            await syncPending()
          }
        }
      },
      fail: () => {
        console.log('[Sync] 启动时离线，等待网络恢复')
      }
    })
  } catch (e) {
    console.warn('[Sync] 启动网络检查失败:', e.message)
  }
}

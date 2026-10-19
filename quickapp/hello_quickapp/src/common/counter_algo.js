/**
 * counter_algo.js — 仰卧起坐 / 引体向上 周期性动作计数算法（前端可运行版）
 *
 * 与 native sport_algo.c 思路一致，并针对“误差 < 10%”做了四项增强：
 *   1. 一阶低通滤波（EMA）    抑制 IMU 噪声引起的抖动误触发
 *   2. 阈值滞回               上升越过 up_th 进入高位，回落越过 down_th 才计一次
 *   3. 最小动作幅度校验       峰-谷差 >= minAmp 才计，过滤小幅晃动
 *   4. 最小动作间隔(refractory) 防止一次起伏被重复计数
 *
 * 坐标系约定（与 native 一致）：
 *   设备静止竖直时 az≈1g；仰卧起坐俯仰角 pitch = atan2(-ay, az) * 180/PI
 *   引体向上取去重力后的加速度幅值 dyn = |a| - 1（单位 g）
 *
 * 数据来源由 test.ux 负责接线：优先 @system.sensor 真实 IMU，
 * 不可用时由内置仿真信号驱动，以保证算法逻辑可被验证且页面始终可用。
 */

/* 一阶低通（指数滑动平均），首值直接赋值 */

function makeLowPass(alpha)
{
  var prev = null
  return function (x)
  {
    if (prev === null)
      {
        prev = x
      }
    else
      {
        prev = prev + alpha * (x - prev)
      }
    return prev
  }
}

/* 通用周期计数器：滞回 + 幅度校验 + 不应期 */

function RepCounter(upTh, downTh, refractoryMs, minAmp)
{
  this.upTh = upTh
  this.downTh = downTh
  this.refractoryMs = refractoryMs || 250
  this.minAmp = (minAmp === undefined) ? 0.15 : minAmp
  this.state = 0                 /* 0 = 等待上升过阈, 1 = 等待回落过阈 */
  this.count = 0
  this.lastCrossMs = -100000
  this.peak = 0                  /* 当前高位峰值 */
  this.valley = 0                /* 当前低位谷值 */
}

RepCounter.prototype.feed = function (value, tMs)
{
  if (this.state === 0)
    {
      /* 等待上升：追踪峰值 */
      if (this.peak === 0 || value > this.peak)
        {
          this.peak = value
        }
      if (value > this.upTh)
        {
          this.state = 1
        }
    }
  else
    {
      /* 等待回落：追踪谷值 */
      if (this.valley === 0 || value < this.valley)
        {
          this.valley = value
        }
      if (value < this.downTh)
        {
          var dt = tMs - this.lastCrossMs
          if ((this.peak - this.valley) >= this.minAmp && dt >= this.refractoryMs)
            {
              this.count++
              this.lastCrossMs = tMs
            }
          this.state = 0
          this.peak = 0
          this.valley = 0
        }
    }
}

/* 仰卧起坐：腰部/手腕俯仰角驱动（单位：度） */

function SitUpCounter()
{
  this.lp = makeLowPass(0.25)
  this.rep = new RepCounter(50.0, 20.0, 300, 15.0)
}

SitUpCounter.prototype.feedImu = function (ax, ay, az, tMs)
{
  var pitch = Math.atan2(-ay, az) * 180.0 / Math.PI
  this.rep.feed(this.lp(pitch), tMs)
}

SitUpCounter.prototype.getCount = function ()
{
  return this.rep.count
}

/* 引体向上：去重力加速度幅值驱动（单位：g） */

function PullUpCounter()
{
  this.lp = makeLowPass(0.20)
  this.rep = new RepCounter(0.30, -0.20, 400, 0.15)
}

PullUpCounter.prototype.feedImu = function (ax, ay, az, tMs)
{
  var mag = Math.sqrt(ax * ax + ay * ay + az * az)
  var dyn = mag - 1.0
  this.rep.feed(this.lp(dyn), tMs)
}

PullUpCounter.prototype.getCount = function ()
{
  return this.rep.count
}

export default {
  SitUpCounter: SitUpCounter,
  PullUpCounter: PullUpCounter,
  createSitUp: function () { return new SitUpCounter() },
  createPullUp: function () { return new PullUpCounter() }
}

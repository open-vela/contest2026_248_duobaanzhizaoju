/****************************************************************************
 * apps/examples/fitness/sport_algo.c
 *
 * 多运动项目支撑层实现：项目注册表 + 各项目核心测量算法 + 传感器仿真。
 *
 * 真实硬件接入点：把 sport_sim_sample() 替换为 LSM6DS3TR-C 驱动读数即可，
 * 算法层（sport_algo_feed）无需改动。
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "sport_algo.h"
#include "ui_common.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PI_F        3.14159265f
#define DEG2RAD(d)  ((d) * PI_F / 180.0f)
#define RAD2DEG(r)  ((r) * 180.0f / PI_F)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* 体测项目注册表（顺序即选择页展示顺序） */

static const sport_profile_t g_profiles[SPORT_PROFILE_MAX] =
{
  {
    TEST_TYPE_JUMP_ROPE, "跳绳", LV_SYMBOL_LOOP, "60秒计数", "次",
    "手腕 IMU Z 轴加速度峰谷计数",
    SPORT_METRIC_REPS, 60, 0x2DD4BF, false, true
  },
  {
    TEST_TYPE_50M_RUN, "50米跑", LV_SYMBOL_FAST_FORWARD, "计时冲刺", "秒",
    "起跑检测 + 步频分析 + 冲刺检测",
    SPORT_METRIC_TIME, 50, 0x2DD4BF, true, true
  },
  {
    TEST_TYPE_800M_RUN, "800米跑", LV_SYMBOL_PLAY, "耐力测试", "公里",
    "GPS 轨迹 + 配速 + 心率区间分析",
    SPORT_METRIC_TIME, 800, 0x2DD4BF, true, true
  },
  {
    TEST_TYPE_SIT_UP, "仰卧起坐", LV_SYMBOL_REFRESH, "60秒计数", "个",
    "腰部俯仰角阈值 + 节奏计数",
    SPORT_METRIC_REPS, 60, 0x2DD4BF, false, true
  },
  {
    TEST_TYPE_PULL_UP, "引体向上", LV_SYMBOL_MINUS, "次数测试", "个",
    "手腕加速度上下周期计数",
    SPORT_METRIC_REPS, 0, 0x2DD4BF, false, true
  },
  {
    TEST_TYPE_LONG_JUMP, "立定跳远", LV_SYMBOL_UP, "距离测量", "米",
    "起跳检测 + 腾空时间估算距离",
    SPORT_METRIC_DISTANCE, 0, 0x2DD4BF, false, false
  },
  {
    TEST_TYPE_1000M_RUN, "1000米跑", LV_SYMBOL_GPS, "耐力测试", "公里",
    "GPS 轨迹 + 配速 + 心率区间分析",
    SPORT_METRIC_TIME, 1000, 0x2DD4BF, true, true
  }
};

static int g_selected = 0;          /* 当前选中项目索引 */

/* 仿真用：立定跳远本次试跳的腾空时间（会话内保持一致） */

static float g_sim_flight_ms = 650.0f;

/****************************************************************************
 * Private Functions — 工具
 ****************************************************************************/

static float rand_noise(float amp)
{
  return ((float)(rand() % 1000) / 1000.0f - 0.5f) * 2.0f * amp;
}

static float vec_mag(const imu_sample_t *s)
{
  return sqrtf(s->ax * s->ax + s->ay * s->ay + s->az * s->az);
}

/****************************************************************************
 * Private Functions — 跳绳峰谷计数（技术方案 2.4 自研算法）
 ****************************************************************************/

static void feed_rope(sport_algo_t *a, const imu_sample_t *s)
{
  rope_counter_t *c = &a->rope;
  float v = s->az;
  float hi = c->threshold + 0.35f;          /* 阈值滞回上限 */
  float lo = c->threshold - 0.35f;          /* 阈值滞回下限 */

  /* 维护窗口极值，用于动态阈值 (max + min) / 2 */

  if (v > c->run_max) c->run_max = v;
  if (v < c->run_min) c->run_min = v;

  if (c->state == 0)                        /* WAIT_PEAK：追踪波峰 */
    {
      if (v > c->last_peak_val)
        {
          c->last_peak_val = v;
        }

      if (v < lo && c->last_peak_val > hi)
        {
          /* 峰值确认且回落到阈值之下 -> 转入等谷 */

          c->state = 1;
          c->last_valley_val = v;
        }
    }
  else                                      /* WAIT_VALLEY：追踪波谷 */
    {
      if (v < c->last_valley_val)
        {
          c->last_valley_val = v;
        }

      if (v > hi)
        {
          /* 回到阈值之上：完成一次 峰 -> 谷 -> 峰 循环 */

          if ((c->last_peak_val - c->last_valley_val) > ROPE_MIN_AMPLITUDE_G)
            {
              c->count++;
              c->since_update++;

              /* 每 50 次刷新一次动态阈值 */

              if (c->since_update >= ROPE_THRESHOLD_UPDATE)
                {
                  c->threshold = (c->run_max + c->run_min) * 0.5f;
                  c->since_update = 0;
                  c->run_max = -100.0f;
                  c->run_min = 100.0f;
                }
            }

          c->state = 0;
          c->last_peak_val = v;
        }
    }
}

/****************************************************************************
 * Private Functions — 通用周期计数（仰卧起坐 / 引体向上）
 ****************************************************************************/

static void feed_rep(sport_algo_t *a, float raw, float dt)
{
  rep_counter_t *c = &a->rep;

  /* 一阶低通（EMA）抑制 IMU 噪声误触发 */
  if (c->lp_alpha > 0.0f)
    {
      if (!c->lp_init)
        {
          c->lp_val = raw;
          c->lp_init = 1;
        }
      else
        {
          c->lp_val += c->lp_alpha * (raw - c->lp_val);
        }
    }
  else
    {
      c->lp_val = raw;
    }

  float value = c->lp_val;
  c->value = value;
  c->t_ms += dt;

  if (c->state == 0)
    {
      /* 等待上升：追踪峰值 */
      if (c->peak == 0.0f || value > c->peak)
        {
          c->peak = value;
        }
      if (value > c->up_th)
        {
          c->state = 1;
        }
    }
  else
    {
      /* 等待回落：追踪谷值 */
      if (c->valley == 0.0f || value < c->valley)
        {
          c->valley = value;
        }
      if (value < c->down_th)
        {
          float ddt = c->t_ms - c->last_cross_ms;
          if ((c->peak - c->valley) >= c->min_amp && ddt >= c->refractory_ms)
            {
              c->count++;
              c->last_cross_ms = c->t_ms;
            }
          c->state = 0;
          c->peak = 0.0f;
          c->valley = 0.0f;
        }
    }
}

/****************************************************************************
 * Private Functions — 立定跳远
 ****************************************************************************/

static void feed_long_jump(sport_algo_t *a, const imu_sample_t *s, float dt)
{
  long_jump_t *l = &a->ljump;
  float mag = vec_mag(s);

  if (l->done)
    {
      return;
    }

  if (l->phase == 0)                        /* 准备：等起跳冲击 */
    {
      if (mag > LONG_JUMP_TAKEOFF_G)
        {
          l->phase = 1;
          l->airborne = false;
          l->air_ms = 0.0f;
        }
    }
  else if (l->phase == 1)                   /* 腾空：累计失重时间 */
    {
      if (!l->airborne)
        {
          if (mag < LONG_JUMP_FREE_G)
            {
              l->airborne = true;           /* 真正离地 */
            }
        }
      else
        {
          if (mag > LONG_JUMP_TAKEOFF_G)
            {
              /* 落地冲击：由腾空时间估算水平距离 */

              float t = l->air_ms / 1000.0f;
              l->flight_ms = l->air_ms;
              l->distance_cm = (int32_t)(LONG_JUMP_K * t * t * 100.0f);
              l->phase = 2;
              l->done = true;
              return;
            }

          l->air_ms += dt;
        }
    }
}

/****************************************************************************
 * Private Functions — 跑步（50m / 800m / 1000m）
 ****************************************************************************/

static void feed_run(sport_algo_t *a, const imu_sample_t *s, float dt)
{
  run_track_t *r = &a->run;
  float v = s->az;

  if (r->done)
    {
      return;
    }

  if (r->refractory_ms > 0)
    {
      r->refractory_ms -= (int)dt;
    }

  /* 步峰检测：阈值滞回 + 不应期，避免一次触地被重复计数 */

  float hi = 1.0f + 0.55f * r->stride_m;    /* 随步幅缩放的冲击门限 */
  float lo = 1.0f + 0.15f * r->stride_m;

  if (!r->armed)
    {
      if (v > hi)
        {
          r->armed = true;
        }
    }
  else if (v < lo)
    {
      r->armed = false;

      if (r->refractory_ms <= 0)
        {
          if (r->phase == 0)
            {
              /* 起跑检测：第一个有效步峰即视为起跑 */

              r->phase = 1;
              r->elapsed_ms = 0.0f;
              r->last_step_ms = 0.0f;
            }
          else
            {
              float interval = r->elapsed_ms - r->last_step_ms;
              if (interval > 50.0f)
                {
                  r->speed_mps = r->stride_m / (interval / 1000.0f);
                  if (r->speed_mps > r->peak_speed_mps)
                    {
                      r->peak_speed_mps = r->speed_mps;
                    }

                  if (r->speed_mps > RUN_SPRINT_SPEED_MPS)
                    {
                      r->sprint = true;     /* 冲刺检测 */
                    }
                }
            }

          r->steps++;
          r->last_step_ms = r->elapsed_ms;
          r->refractory_ms = RUN_STEP_REFRACTORY_MS;
          r->distance_m = (float)r->steps * r->stride_m;
        }
    }

  if (r->phase == 1)
    {
      r->elapsed_ms += dt;

      /* 到达目标距离 -> 完赛 */

      if ((uint32_t)a->profile->target > 0 &&
          r->distance_m >= (float)a->profile->target)
        {
          r->phase = 2;
          r->done = true;
          r->finish_ms = r->elapsed_ms;
          r->distance_m = (float)a->profile->target;
        }
    }
}

/****************************************************************************
 * Private Functions — 传感器仿真
 ****************************************************************************/

/** 跳绳：手腕 Z 轴 ~2.55Hz 正弦（约 153 次/分钟） */

static void sim_jump_rope(imu_sample_t *s, float t_ms)
{
  float f = 2.55f + 0.25f * sinf(2.0f * PI_F * t_ms / 7000.0f);
  float w = 2.0f * PI_F * f * t_ms / 1000.0f;
  float n = rand_noise(0.12f);

  s->az = 1.0f + 2.1f * sinf(w) + n;
  s->ax = 0.25f * sinf(w * 0.5f) + n * 0.5f;
  s->ay = 0.30f * cosf(w * 0.5f) + n * 0.5f;
  s->gx = 0.0f;
  s->gy = 0.0f;
  s->gz = 120.0f * cosf(w);
}

/** 仰卧起坐：腰部俯仰角 2~68 度，~0.78Hz（约 47 次/分钟） */

static void sim_sit_up(imu_sample_t *s, float t_ms)
{
  float w = 2.0f * PI_F * 0.78f * t_ms / 1000.0f;
  float pitch = DEG2RAD(35.0f + 33.0f * sinf(w));
  float n = rand_noise(0.05f);

  s->ay = -sinf(pitch) + n;
  s->az = cosf(pitch) + n;
  s->ax = n;
  s->gx = 0.0f;
  s->gy = 150.0f * cosf(w);
  s->gz = 0.0f;
}

/** 引体向上：手腕垂直加速度 ~0.2Hz（约 12 次/分钟） */

static void sim_pull_up(imu_sample_t *s, float t_ms)
{
  float w = 2.0f * PI_F * 0.20f * t_ms / 1000.0f;
  float n = rand_noise(0.06f);

  s->az = 1.0f + 0.62f * sinf(w) + n;
  s->ax = n * 0.5f;
  s->ay = n * 0.5f;
  s->gx = 0.0f;
  s->gy = 0.0f;
  s->gz = 40.0f * cosf(w);
}

/** 立定跳远：站立 -> 起跳冲击 -> 腾空失重 -> 落地冲击 */

static void sim_long_jump(imu_sample_t *s, float t_ms)
{
  const float t_takeoff = 900.0f;
  const float t_free = t_takeoff + 80.0f;
  const float t_land = t_free + g_sim_flight_ms;
  float n = rand_noise(0.08f);

  if (t_ms < t_takeoff)
    {
      s->az = 1.0f + n;                     /* 站立准备 */
    }
  else if (t_ms < t_free)
    {
      s->az = 3.2f + n;                     /* 蹬地起跳冲击 */
    }
  else if (t_ms < t_land)
    {
      s->az = 0.05f + n * 0.4f;             /* 腾空失重 */
    }
  else if (t_ms < t_land + 120.0f)
    {
      s->az = 5.5f + n;                     /* 落地冲击 */
    }
  else
    {
      s->az = 1.0f + n;                     /* 落地站定 */
    }

  s->ax = n * 0.5f;
  s->ay = n * 0.5f;
  s->gx = 0.0f;
  s->gy = 0.0f;
  s->gz = 0.0f;
}

/** 跑步：起跑前静立 0.7s，之后按步频产生周期性垂直冲击 */

static void sim_run(imu_sample_t *s, float t_ms, float cadence_hz,
                    float amp)
{
  const float ready_ms = 700.0f;
  float n = rand_noise(0.10f);

  if (t_ms < ready_ms)
    {
      s->az = 1.0f + n * 0.4f;              /* 起跑前静立 */
    }
  else
    {
      float w = 2.0f * PI_F * cadence_hz * (t_ms - ready_ms) / 1000.0f;
      s->az = 1.0f + amp * sinf(w) + n;
    }

  s->ax = n * 0.5f;
  s->ay = n * 0.5f;
  s->gx = 0.0f;
  s->gy = 0.0f;
  s->gz = 0.0f;
}

/****************************************************************************
 * Public Functions — 项目注册表
 ****************************************************************************/

int sport_profile_count(void)
{
  return SPORT_PROFILE_MAX;
}

const sport_profile_t *sport_profile_get(int idx)
{
  if (idx < 0 || idx >= SPORT_PROFILE_MAX)
    {
      return NULL;
    }

  return &g_profiles[idx];
}

const sport_profile_t *sport_profile_find(const char *test_type)
{
  if (test_type == NULL)
    {
      return NULL;
    }

  for (int i = 0; i < SPORT_PROFILE_MAX; i++)
    {
      if (strcmp(g_profiles[i].type, test_type) == 0)
        {
          return &g_profiles[i];
        }
    }

  return NULL;
}

void sport_profile_select(int idx)
{
  if (idx >= 0 && idx < SPORT_PROFILE_MAX)
    {
      g_selected = idx;
    }
}

int sport_profile_selected(void)
{
  return g_selected;
}

const sport_profile_t *sport_profile_current(void)
{
  return &g_profiles[g_selected];
}

/****************************************************************************
 * Public Functions — 算法
 ****************************************************************************/

void sport_algo_init(sport_algo_t *algo, const char *test_type)
{
  const sport_profile_t *p;

  memset(algo, 0, sizeof(*algo));

  p = sport_profile_find(test_type);
  if (p == NULL)
    {
      p = &g_profiles[0];                   /* 未知项目回退到跳绳 */
    }

  algo->profile = p;

  /* 跳绳动态阈值初值：静止时 Z 轴约为 1g */

  algo->rope.threshold = 1.0f;
  algo->rope.last_peak_val = -100.0f;
  algo->rope.last_valley_val = 100.0f;
  algo->rope.run_max = -100.0f;
  algo->rope.run_min = 100.0f;

  /* 立定跳远：每次试跳随机腾空时间 600~700ms（约 1.89~2.57m） */

  g_sim_flight_ms = 600.0f + (float)(rand() % 101);

  /* 跑步参数按项目区分 */

  if (strcmp(p->type, TEST_TYPE_50M_RUN) == 0)
    {
      algo->run.stride_m = RUN_STRIDE_50M_M;
    }
  else
    {
      algo->run.stride_m = RUN_STRIDE_LONG_M;
    }

  /* 周期计数阈值（与 quickapp counter_algo.js 一致） */

  if (strcmp(p->type, TEST_TYPE_SIT_UP) == 0)
    {
      algo->rep.up_th = SIT_UP_UP_DEG;
      algo->rep.down_th = SIT_UP_DOWN_DEG;
      algo->rep.refractory_ms = SIT_UP_REFRACTORY_MS;
      algo->rep.min_amp = SIT_UP_MIN_AMP_DEG;
      algo->rep.lp_alpha = SIT_UP_LP_ALPHA;
    }
  else if (strcmp(p->type, TEST_TYPE_PULL_UP) == 0)
    {
      algo->rep.up_th = PULL_UP_UP_G;
      algo->rep.down_th = PULL_UP_DOWN_G;
      algo->rep.refractory_ms = PULL_UP_REFRACTORY_MS;
      algo->rep.min_amp = PULL_UP_MIN_AMP_G;
      algo->rep.lp_alpha = PULL_UP_LP_ALPHA;
    }
}

void sport_algo_feed(sport_algo_t *algo, const imu_sample_t *sample)
{
  const char *t;
  float dt = (float)SENSOR_DT_MS_TEST;

  if (algo->profile == NULL)
    {
      return;
    }

  algo->sample_count++;
  algo->time_ms += dt;
  t = algo->profile->type;

  if (strcmp(t, TEST_TYPE_JUMP_ROPE) == 0)
    {
      feed_rope(algo, sample);
    }
  else if (strcmp(t, TEST_TYPE_SIT_UP) == 0)
    {
      /* 腰部俯仰角 = atan2(-ay, az) */

      feed_rep(algo, RAD2DEG(atan2f(-sample->ay, sample->az)), dt);
    }
  else if (strcmp(t, TEST_TYPE_PULL_UP) == 0)
    {
      /* 手腕：去重力后的加速度幅值 */

      feed_rep(algo, vec_mag(sample) - 1.0f, dt);
    }
  else if (strcmp(t, TEST_TYPE_LONG_JUMP) == 0)
    {
      feed_long_jump(algo, sample, dt);
    }
  else
    {
      feed_run(algo, sample, dt);
    }

  sport_algo_sync(algo);
}

/** 把各子算法的输出同步到统一主指标 */

void sport_algo_sync(sport_algo_t *algo)
{
  const char *t;

  if (algo->profile == NULL)
    {
      return;
    }

  t = algo->profile->type;

  if (strcmp(t, TEST_TYPE_JUMP_ROPE) == 0)
    {
      algo->count = algo->rope.count;
      if (algo->time_ms > 1000.0f)
        {
          algo->cadence = (int32_t)((float)algo->count * 60000.0f /
                                    algo->time_ms);
        }
    }
  else if (strcmp(t, TEST_TYPE_SIT_UP) == 0 ||
           strcmp(t, TEST_TYPE_PULL_UP) == 0)
    {
      algo->count = algo->rep.count;
      if (algo->time_ms > 1000.0f)
        {
          algo->cadence = (int32_t)((float)algo->count * 60000.0f /
                                    algo->time_ms);
        }
    }
  else if (strcmp(t, TEST_TYPE_LONG_JUMP) == 0)
    {
      algo->distance_cm = algo->ljump.distance_cm;
      algo->finished = algo->ljump.done;
    }
  else
    {
      run_track_t *r = &algo->run;

      algo->distance_cm = (int32_t)(r->distance_m * 100.0f);
      algo->result_sec = r->phase == 2 ?
                         (r->finish_ms / 1000.0f) :
                         (r->elapsed_ms / 1000.0f);
      algo->cadence = (r->elapsed_ms > 1000.0f) ?
                      (int32_t)((float)r->steps * 60000.0f / r->elapsed_ms) :
                      0;
      algo->pace_sec_per_km = (r->distance_m > 1.0f) ?
                              (int32_t)(r->elapsed_ms / 1000.0f * 1000.0f /
                                        r->distance_m) : 0;
      algo->finished = r->done;
    }

  /* 计数型项目到达预置时长后自动结束 */

  if (algo->profile->metric == SPORT_METRIC_REPS &&
      algo->profile->target > 0 &&
      algo->time_ms >= (float)algo->profile->target * 1000.0f)
    {
      algo->finished = true;
    }
}

/****************************************************************************
 * Public Functions — 传感器仿真
 ****************************************************************************/

void sport_sim_sample(const sport_algo_t *algo, imu_sample_t *out)
{
  const char *t;
  float t_ms;

  if (algo->profile == NULL)
    {
      return;
    }

  t = algo->profile->type;
  t_ms = algo->time_ms;

  if (strcmp(t, TEST_TYPE_JUMP_ROPE) == 0)
    {
      sim_jump_rope(out, t_ms);
    }
  else if (strcmp(t, TEST_TYPE_SIT_UP) == 0)
    {
      sim_sit_up(out, t_ms);
    }
  else if (strcmp(t, TEST_TYPE_PULL_UP) == 0)
    {
      sim_pull_up(out, t_ms);
    }
  else if (strcmp(t, TEST_TYPE_LONG_JUMP) == 0)
    {
      sim_long_jump(out, t_ms);
    }
  else if (strcmp(t, TEST_TYPE_50M_RUN) == 0)
    {
      sim_run(out, t_ms, RUN_CADENCE_50M_HZ, 2.2f);
    }
  else
    {
      sim_run(out, t_ms, RUN_CADENCE_LONG_HZ, 1.6f);
    }
}

/****************************************************************************
 * Public Functions — 成绩格式化
 ****************************************************************************/

/** 实时主指标格式化，如 "128" / "2.21 m" / "07.30" */

void sport_format_value(const sport_profile_t *p, int32_t count,
                        int32_t distance_cm, float time_sec,
                        char *buf, int len)
{
  if (p == NULL)
    {
      snprintf(buf, len, "--");
      return;
    }

  switch (p->metric)
    {
      case SPORT_METRIC_DISTANCE:
        snprintf(buf, len, "%.2f 米", (double)distance_cm / 100.0);
        break;

      case SPORT_METRIC_TIME:
        if (time_sec >= 60.0f)
          {
            int m = (int)time_sec / 60;
            int s = (int)time_sec % 60;
            snprintf(buf, len, "%d'%02d\"", m, s);
          }
        else
          {
            snprintf(buf, len, "%.1f 秒", (double)time_sec);
          }
        break;

      default:
        snprintf(buf, len, "%ld%s", (long)count, p->unit);
        break;
    }
}

/** 历史/详情记录格式化 */

void sport_format_record(const fitness_record_t *rec, char *buf, int len)
{
  const sport_profile_t *p = sport_profile_find(rec->test_type);

  if (p != NULL && p->metric == SPORT_METRIC_TIME &&
      rec->distance_cm > 0)
    {
      /* 跑步同时展示成绩时间与距离 */

      char val[24];
      sport_format_value(p, rec->count, rec->distance_cm, rec->time_sec,
                         val, sizeof(val));
      snprintf(buf, len, "%s / %.0f米", val,
               (double)rec->distance_cm / 100.0);
      return;
    }

  sport_format_value(p, rec->count, rec->distance_cm, rec->time_sec,
                     buf, len);
}

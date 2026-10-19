/****************************************************************************
 * apps/examples/fitness/sport_algo.h
 *
 * 多运动项目支撑层：体测项目注册表 + 各项目核心测量算法。
 *
 * 依据《中学生体测穿戴设备_技术方案》2.3 / 2.4 节实现：
 *
 *   项目        采集方式              核心算法
 *   ---------------------------------------------------------------
 *   jump_rope   手腕 IMU Z 轴加速度    峰谷检测计数（自研，动态阈值）
 *   50m_run     IMU 加速度 + GPS      起跑检测 + 步频分析 + 冲刺检测
 *   800m_run    GPS 轨迹 + 心率        配速计算 + 心率区间分析
 *   1000m_run   GPS 轨迹 + 心率        配速计算 + 心率区间分析
 *   long_jump   IMU 腾空姿态          起跳检测 + 腾空时间 -> 距离估算
 *   sit_up      腰部 IMU 俯仰角        角度阈值 + 节奏计数
 *   pull_up     手腕 IMU + 振动检测    上升/下降周期计数
 *
 * 本头文件不依赖 LVGL，可单独被业务逻辑层引用。
 ****************************************************************************/

#ifndef __SPORT_ALGO_H
#define __SPORT_ALGO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IMU 采样率：体测进行中 50Hz，非体测时段 25Hz（技术方案 2.8 省电策略） */

#define SENSOR_HZ_TEST          50
#define SENSOR_HZ_IDLE          25
#define SENSOR_DT_MS_TEST       (1000 / SENSOR_HZ_TEST)

/* 采集线程节拍：每 100ms 喂入 5 个采样点 */

#define COLLECT_TICK_MS         100
#define SAMPLES_PER_TICK        (SENSOR_HZ_TEST * COLLECT_TICK_MS / 1000)

/* 物理常量 */

#define GRAVITY_G               9.80665f

/* 立定跳远：由腾空时间估算水平距离  distance_m = K * t_air^2
 * K 由 45 度起飞模型标定：v = g*t/2，distance = 1.07*v*t ≈ 5.24*t^2
 * 校验：t = 0.65s -> 2.21m（初三男生良好水平）
 */

#define LONG_JUMP_K             5.24f
#define LONG_JUMP_TAKEOFF_G     2.5f    /* 起跳/落地冲击判据 (g) */
#define LONG_JUMP_FREE_G        0.40f   /* 腾空失重判据 (g) */

/* 跑步：步频/步幅经验参数 */

#define RUN_STRIDE_50M_M        1.56f   /* 50 米跑冲刺步幅 (m) */
#define RUN_CADENCE_50M_HZ      4.4f    /* 50 米跑步频 (步/秒) */
#define RUN_STRIDE_LONG_M       1.45f   /* 中长跑步幅 (m) */
#define RUN_CADENCE_LONG_HZ     3.0f    /* 中长跑步频 (步/秒) */
#define RUN_STEP_REFRACTORY_MS  180     /* 步峰检测不应期 (ms) */
#define RUN_SPRINT_SPEED_MPS    6.5f    /* 冲刺判定速度 (m/s) */

/* 计数型项目角度/加速度阈值 */

#define SIT_UP_UP_DEG           50.0f   /* 仰卧起坐：起身俯仰角阈值 */
#define SIT_UP_DOWN_DEG         20.0f   /* 仰卧起坐：躺回俯仰角阈值 */
#define PULL_UP_UP_G            0.30f   /* 引体向上：上拉加速度阈值 (g) */
#define PULL_UP_DOWN_G         -0.20f   /* 引体向上：下放加速度阈值 (g) */

/* 周期计数鲁棒性参数（与 quickapp counter_algo.js 保持一致） */
#define SIT_UP_REFRACTORY_MS    300.0f  /* 仰卧起坐：最小动作间隔 */
#define SIT_UP_MIN_AMP_DEG      15.0f   /* 仰卧起坐：最小峰谷幅度 */
#define SIT_UP_LP_ALPHA         0.25f   /* 仰卧起坐：低通系数 */
#define PULL_UP_REFRACTORY_MS   400.0f  /* 引体向上：最小动作间隔 */
#define PULL_UP_MIN_AMP_G       0.15f   /* 引体向上：最小峰谷幅度 */
#define PULL_UP_LP_ALPHA        0.20f   /* 引体向上：低通系数 */

/* 跳绳峰谷检测参数 */

#define ROPE_MIN_AMPLITUDE_G    1.20f   /* 有效峰谷最小幅度，抑制抖动误计 */
#define ROPE_THRESHOLD_UPDATE   50      /* 每 N 次更新一次动态阈值 */

/* 支持的体测项目数量 */

#define SPORT_PROFILE_MAX       7

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* 六轴 IMU 采样（LSM6DS3TR-C） */

typedef struct
{
  float ax;                     /* X 轴加速度 (g) */
  float ay;                     /* Y 轴加速度 (g) */
  float az;                     /* Z 轴加速度 (g) */
  float gx;                     /* X 轴角速度 (dps) */
  float gy;                     /* Y 轴角速度 (dps) */
  float gz;                     /* Z 轴角速度 (dps) */
} imu_sample_t;

/* 成绩度量类型：决定主指标如何展示与评分 */

typedef enum
{
  SPORT_METRIC_REPS = 0,        /* 计数型：跳绳 / 仰卧起坐 / 引体向上 */
  SPORT_METRIC_DISTANCE,        /* 距离型：立定跳远 */
  SPORT_METRIC_TIME             /* 计时型：50m / 800m / 1000m 跑 */
} sport_metric_t;

/* 体测项目元数据 */

typedef struct
{
  const char    *type;          /* TEST_TYPE_xxx 协议标识 */
  const char    *name;          /* 展示名，如 "跳绳" */
  const char    *icon;          /* 列表图标（LVGL 符号字体） */
  const char    *hint;          /* 选择页副标题，如 "60秒计数" */
  const char    *unit;          /* 主指标单位，如 "次" / "米" / "公里" */
  const char    *algo_note;     /* 算法说明 */
  sport_metric_t metric;        /* 度量类型 */
  uint32_t       target;        /* 计数型=默认时长(s)；计时/距离型=目标距离(m) */
  uint32_t       color_hex;     /* 项目主题色 */
  bool           needs_gps;     /* 是否需要 GPS 扩展模块 */
  bool           needs_hr;      /* 是否需要心率传感器 */
} sport_profile_t;

/* 跳绳峰谷计数器（技术方案 2.4 自研算法） */

typedef struct
{
  float threshold;              /* 动态阈值 = (max + min) / 2 */
  float last_peak_val;          /* 上一个峰值 */
  float last_valley_val;        /* 上一个谷值 */
  int   count;                  /* 跳绳次数 */
  int   state;                  /* 0 = WAIT_PEAK, 1 = WAIT_VALLEY */
  int   since_update;           /* 距上次阈值更新的计数 */
  float run_max;                /* 窗口内最大值 */
  float run_min;                /* 窗口内最小值 */
} rope_counter_t;

/* 通用周期计数器（仰卧起坐俯仰角 / 引体向上加速度），带滞回消抖 */

typedef struct
{
  float value;                  /* 当前观测量（角度 deg 或 加速度 g） */
  int   state;                  /* 0 = 等待上升过阈, 1 = 等待回落过阈 */
  int   count;                  /* 完成次数 */
  float up_th;                  /* 上升阈值 */
  float down_th;                /* 回落阈值 */
  float refractory_ms;          /* 最小动作间隔（防一次起伏重复计数） */
  float min_amp;                /* 最小峰谷幅度（过滤小幅晃动） */
  float lp_alpha;               /* 一阶低通系数，0 = 不滤波 */
  float peak;                   /* 当前高位峰值 */
  float valley;                 /* 当前低位谷值 */
  float t_ms;                   /* 累计时间 */
  float last_cross_ms;          /* 上次计数时刻 */
  float lp_val;                 /* 低通当前值 */
  int   lp_init;                /* 低通是否已初始化 */
} rep_counter_t;

/* 立定跳远检测器 */

typedef struct
{
  int   phase;                  /* 0 = 准备, 1 = 腾空, 2 = 落地 */
  bool  airborne;               /* 是否进入真正失重段 */
  float air_ms;                 /* 腾空累计时间 (ms) */
  float flight_ms;              /* 最终腾空时间 (ms) */
  float distance_cm;            /* 估算距离 (cm) */
  bool  done;                   /* 本次试跳结束 */
} long_jump_t;

/* 跑步跟踪器（起跑检测 + 步频 + 距离 + 配速 + 冲刺） */

typedef struct
{
  int   phase;                  /* 0 = 等待起跑, 1 = 跑步中, 2 = 已完成 */
  int   steps;                  /* 步数 */
  float stride_m;               /* 步幅 (m) */
  float distance_m;             /* 累计距离 (m) */
  float elapsed_ms;             /* 起跑后累计时间 (ms) */
  float finish_ms;              /* 完赛时间 (ms) */
  int   cadence;                /* 步频 (步/分钟) */
  float speed_mps;              /* 瞬时速度 (m/s) */
  float peak_speed_mps;         /* 峰值速度 (m/s) */
  bool  sprint;                 /* 是否检测到冲刺 */
  bool  done;                   /* 到达目标距离 */
  bool  armed;                  /* 步峰检测滞回状态 */
  int   refractory_ms;          /* 步峰检测不应期 */
  float last_step_ms;           /* 上一步时刻 */
} run_track_t;

/* 单个运动会话的算法状态 */

typedef struct
{
  const sport_profile_t *profile;   /* 当前项目 */
  rope_counter_t        rope;
  rep_counter_t         rep;
  long_jump_t           ljump;
  run_track_t           run;

  uint32_t              sample_count;   /* 已处理采样数 */
  float                 time_ms;        /* 会话累计时间 (ms) */

  int32_t               count;          /* 主指标：次数 */
  int32_t               distance_cm;    /* 主指标：距离 (cm) */
  float                 result_sec;     /* 主指标：用时 (s) */
  int32_t               cadence;        /* 步频 / 动作频率 (次每分钟) */
  int32_t               pace_sec_per_km;/* 配速 (秒每公里) */
  bool                  finished;       /* 达标自动结束 */
} sport_algo_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* 项目注册表 */

int  sport_profile_count(void);
const sport_profile_t *sport_profile_get(int idx);
const sport_profile_t *sport_profile_find(const char *test_type);

/* 当前选中项目（由选择页设置） */

void sport_profile_select(int idx);
int  sport_profile_selected(void);
const sport_profile_t *sport_profile_current(void);

/* 算法生命周期 */

void sport_algo_init(sport_algo_t *algo, const char *test_type);
void sport_algo_feed(sport_algo_t *algo, const imu_sample_t *sample);
void sport_algo_sync(sport_algo_t *algo);

/* 传感器仿真（未接入真实 IMU 时用于驱动算法演示） */

void sport_sim_sample(const sport_algo_t *algo, imu_sample_t *out);

#endif /* __SPORT_ALGO_H */

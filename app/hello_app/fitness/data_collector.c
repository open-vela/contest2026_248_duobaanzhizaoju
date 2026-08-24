/****************************************************************************
 * apps/examples/fitness/data_collector.c
 *
 * Data collector: simulated sensor data for fitness tests,
 * score calculation, and HTTP data upload.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "data_collector.h"
#include "http_client.h"
#include "ui_manager.h"

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COLLECT_INTERVAL_MS  1000   /* 1 second */
#define SIM_HR_BASE          72

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_t g_collector_thread;
static bool g_running = false;
static app_context_t g_app_ctx;
static uint32_t g_msg_seq = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint64_t get_timestamp_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/** Simulate sensor data for the current sport session */

static void simulate_sport_data(sport_data_t *sp)
{
  if (!sp->running || sp->paused)
    {
      return;
    }

  /* Increment time */

  sp->elapsed_sec++;

  /* Simulate heart rate based on exercise duration */

  int base_hr = 72 + (sp->elapsed_sec * 2);
  if (base_hr > 170)
    {
      base_hr = 170;
    }
  int variation = (rand() % 11) - 5;
  sp->heart_rate = base_hr + variation;

  if (sp->heart_rate > sp->max_hr)
    {
      sp->max_hr = sp->heart_rate;
    }

  sp->hr_sum += sp->heart_rate;
  sp->hr_samples++;
  sp->avg_hr = sp->hr_sum / sp->hr_samples;

  /* Classify heart rate zone */

  sp->current_zone = classify_hr(sp->heart_rate);
  sp->hr_zones[sp->current_zone]++;

  /* Simulate jump rope count (increment every ~0.5s when active) */

  if (sp->elapsed_sec > 1 && (sp->elapsed_sec % 1 == 0))
    {
      /* Average ~3 jumps per second */

      sp->count += 2 + (rand() % 3);
    }

  /* Update HR history for waveform */

  sp->hr_history[sp->hr_history_idx] = sp->heart_rate;
  sp->hr_history_idx = (sp->hr_history_idx + 1) % HR_HISTORY_MAX;
}

/** Build fitness record JSON for upload */

static void build_record_json(const fitness_record_t *rec, char *buf,
                                int max_len)
{
  snprintf(buf, max_len,
    "{"
    "\"device_sn\":\"%s\","
    "\"student_id\":\"%s\","
    "\"test_type\":\"%s\","
    "\"count\":%ld,"
    "\"time_sec\":%.1f,"
    "\"score\":%ld,"
    "\"grade\":\"%s\","
    "\"avg_hr\":%ld,"
    "\"max_hr\":%ld,"
    "\"timestamp\":%lu"
    "}",
    CONFIG_EXAMPLES_FITNESS_DEVICE_SN,
    CONFIG_EXAMPLES_FITNESS_STUDENT_ID,
    rec->test_type,
    (long)rec->count,
    (double)rec->time_sec,
    (long)rec->score,
    rec->grade,
    (long)rec->avg_hr,
    (long)rec->max_hr,
    (unsigned long)(get_timestamp_ms() / 1000));
}

/** Upload fitness record to server */

static void upload_record(const fitness_record_t *rec)
{
  char json[512];
  build_record_json(rec, json, sizeof(json));
  int ret = http_upload_fitness_record(CONFIG_EXAMPLES_FITNESS_DEVICE_SN, json);
  g_app_ctx.network_connected = (ret == 0);
}

/** Main data collection thread */

static void *collector_thread(void *arg)
{
  LV_UNUSED(arg);

  while (g_running)
    {
      /* Simulate sport data if session is active */

      if (g_app_ctx.sport.running && !g_app_ctx.sport.paused)
        {
          simulate_sport_data(&g_app_ctx.sport);

          /* Update sport UI */

          ui_manager_update_sport();
        }

      usleep(COLLECT_INTERVAL_MS * 1000);
    }

  return NULL;
}

/** Initialize demo history data */

static void init_demo_data(app_context_t *ctx)
{
  ctx->latest_jump_rope = 175;
  ctx->latest_avg_hr = 150;
  ctx->pass_rate = 100;

  /* Demo history records */

  ctx->history_count = 5;

  strncpy(ctx->history[0].test_type, TEST_TYPE_JUMP_ROPE,
          sizeof(ctx->history[0].test_type) - 1);
  strncpy(ctx->history[0].test_name, "Jump Rope",
          sizeof(ctx->history[0].test_name) - 1);
  ctx->history[0].count = 175;
  ctx->history[0].score = 90;
  strncpy(ctx->history[0].grade, "good", sizeof(ctx->history[0].grade) - 1);

  strncpy(ctx->history[1].test_type, TEST_TYPE_50M_RUN,
          sizeof(ctx->history[1].test_type) - 1);
  strncpy(ctx->history[1].test_name, "50m Run",
          sizeof(ctx->history[1].test_name) - 1);
  ctx->history[1].count = 1;
  ctx->history[1].time_sec = 7.3f;
  ctx->history[1].score = 85;
  strncpy(ctx->history[1].grade, "good", sizeof(ctx->history[1].grade) - 1);

  strncpy(ctx->history[2].test_type, TEST_TYPE_LONG_JUMP,
          sizeof(ctx->history[2].test_type) - 1);
  strncpy(ctx->history[2].test_name, "Long Jump",
          sizeof(ctx->history[2].test_name) - 1);
  ctx->history[2].count = 228;
  ctx->history[2].score = 82;
  strncpy(ctx->history[2].grade, "good", sizeof(ctx->history[2].grade) - 1);

  strncpy(ctx->history[3].test_type, TEST_TYPE_SIT_UP,
          sizeof(ctx->history[3].test_type) - 1);
  strncpy(ctx->history[3].test_name, "Sit-ups",
          sizeof(ctx->history[3].test_name) - 1);
  ctx->history[3].count = 48;
  ctx->history[3].score = 88;
  strncpy(ctx->history[3].grade, "good", sizeof(ctx->history[3].grade) - 1);

  strncpy(ctx->history[4].test_type, TEST_TYPE_PULL_UP,
          sizeof(ctx->history[4].test_type) - 1);
  strncpy(ctx->history[4].test_name, "Pull-ups",
          sizeof(ctx->history[4].test_name) - 1);
  ctx->history[4].count = 12;
  ctx->history[4].score = 78;
  strncpy(ctx->history[4].grade, "pass", sizeof(ctx->history[4].grade) - 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void data_collector_init(void)
{
  memset(&g_app_ctx, 0, sizeof(g_app_ctx));

  /* Set defaults */

  strncpy(g_app_ctx.student_name, STUDENT_NAME,
          sizeof(g_app_ctx.student_name) - 1);
  strncpy(g_app_ctx.student_id, STUDENT_ID,
          sizeof(g_app_ctx.student_id) - 1);
  strncpy(g_app_ctx.grade_class, "Grade 9 Class 2",
          sizeof(g_app_ctx.grade_class) - 1);
  strncpy(g_app_ctx.firmware_ver, "v1.0.0",
          sizeof(g_app_ctx.firmware_ver) - 1);
  strncpy(g_app_ctx.server_url, SERVER_URL,
          sizeof(g_app_ctx.server_url) - 1);
  strncpy(g_app_ctx.device_sn, DEVICE_SN,
          sizeof(g_app_ctx.device_sn) - 1);

  g_app_ctx.settings.bluetooth_on = true;
  g_app_ctx.settings.wifi_on = false;

  /* Initialize demo data */

  init_demo_data(&g_app_ctx);

  /* Initialize HTTP client */

  http_client_init();

  /* Seed random */

  srand((unsigned int)time(NULL));

  /* Start collector thread */

  g_running = true;
  pthread_create(&g_collector_thread, NULL, collector_thread, NULL);
}

void data_collector_stop(void)
{
  g_running = false;
  pthread_join(g_collector_thread, NULL);
}

void data_collector_start_sport(const char *test_type)
{
  sport_data_t *sp = &g_app_ctx.sport;
  memset(sp, 0, sizeof(*sp));
  strncpy(sp->test_type, test_type, sizeof(sp->test_type) - 1);

  /* Set display name */

  if (strcmp(test_type, TEST_TYPE_JUMP_ROPE) == 0)
    {
      strncpy(sp->test_name, "Jump Rope", sizeof(sp->test_name) - 1);
    }
  else if (strcmp(test_type, TEST_TYPE_50M_RUN) == 0)
    {
      strncpy(sp->test_name, "50m Run", sizeof(sp->test_name) - 1);
    }
  else if (strcmp(test_type, TEST_TYPE_SIT_UP) == 0)
    {
      strncpy(sp->test_name, "Sit-ups", sizeof(sp->test_name) - 1);
    }
  else if (strcmp(test_type, TEST_TYPE_PULL_UP) == 0)
    {
      strncpy(sp->test_name, "Pull-ups", sizeof(sp->test_name) - 1);
    }
  else
    {
      strncpy(sp->test_name, test_type, sizeof(sp->test_name) - 1);
    }

  sp->running = true;
  sp->paused = false;
  sp->heart_rate = SIM_HR_BASE;
}

void data_collector_stop_sport(void)
{
  g_app_ctx.sport.running = false;
  g_app_ctx.sport.paused = false;

  /* Update home page data with latest results */

  g_app_ctx.latest_jump_rope = g_app_ctx.sport.count;
  g_app_ctx.latest_avg_hr = g_app_ctx.sport.avg_hr;
}

void data_collector_pause_sport(void)
{
  g_app_ctx.sport.paused = true;
}

void data_collector_resume_sport(void)
{
  g_app_ctx.sport.paused = false;
}

const sport_data_t *data_collector_get_sport_data(void)
{
  return &g_app_ctx.sport;
}

int32_t calculate_score(const char *test_type, int32_t count,
                         float time_sec)
{
  /* Simplified scoring based on national standards */

  if (strcmp(test_type, TEST_TYPE_JUMP_ROPE) == 0)
    {
      /* Jump rope: 180/min = 100, 100/min = 60 */

      if (count >= 180) return 100;
      if (count >= 170) return 95;
      if (count >= 160) return 90;
      if (count >= 140) return 80;
      if (count >= 120) return 70;
      if (count >= 100) return 60;
      if (count >= 80) return 50;
      return count * 50 / 80;
    }
  else if (strcmp(test_type, TEST_TYPE_50M_RUN) == 0)
    {
      /* 50m run: 7.0s = 100, 8.5s = 60 */

      if (time_sec <= 7.0f) return 100;
      if (time_sec <= 7.3f) return 90;
      if (time_sec <= 7.5f) return 80;
      if (time_sec <= 7.8f) return 70;
      if (time_sec <= 8.0f) return 60;
      if (time_sec <= 8.5f) return 50;
      return 40;
    }
  else if (strcmp(test_type, TEST_TYPE_SIT_UP) == 0)
    {
      /* Sit-ups: 52/min = 100, 30/min = 60 */

      if (count >= 52) return 100;
      if (count >= 48) return 90;
      if (count >= 44) return 80;
      if (count >= 38) return 70;
      if (count >= 30) return 60;
      return count * 60 / 30;
    }
  else if (strcmp(test_type, TEST_TYPE_PULL_UP) == 0)
    {
      /* Pull-ups: 15 = 100, 6 = 60 */

      if (count >= 15) return 100;
      if (count >= 13) return 90;
      if (count >= 11) return 80;
      if (count >= 9) return 70;
      if (count >= 6) return 60;
      return count * 60 / 6;
    }

  /* Default: linear scale */

  return (count > 0) ? 60 + (count % 40) : 0;
}

/****************************************************************************
 * Global context accessor + helper functions
 ****************************************************************************/

app_context_t *app_get_context(void)
{
  return &g_app_ctx;
}

const char *get_grade_text(int32_t score)
{
  if (score >= 90) return "Excellent";
  if (score >= 80) return "Good";
  if (score >= 60) return "Pass";
  return "Fail";
}

lv_color_t get_grade_color(int32_t score)
{
  if (score >= 90) return COLOR_GREEN;
  if (score >= 80) return COLOR_GREEN;
  if (score >= 60) return COLOR_YELLOW;
  return COLOR_RED;
}

const char *get_hr_zone_name(hr_zone_t zone)
{
  switch (zone)
    {
      case HR_ZONE_REST:     return "Rest";
      case HR_ZONE_FAT_BURN: return "Fat Burn";
      case HR_ZONE_AEROBIC:  return "Aerobic";
      case HR_ZONE_EXTREME:  return "Extreme";
      default:               return "Unknown";
    }
}

lv_color_t get_hr_zone_color(hr_zone_t zone)
{
  switch (zone)
    {
      case HR_ZONE_REST:     return COLOR_HR_REST;
      case HR_ZONE_FAT_BURN: return COLOR_HR_FAT_BURN;
      case HR_ZONE_AEROBIC:  return COLOR_HR_AEROBIC;
      case HR_ZONE_EXTREME:  return COLOR_HR_EXTREME;
      default:               return COLOR_TEXT_DIM;
    }
}

hr_zone_t classify_hr(int32_t hr)
{
  if (hr < 100) return HR_ZONE_REST;
  if (hr < 140) return HR_ZONE_FAT_BURN;
  if (hr < 170) return HR_ZONE_AEROBIC;
  return HR_ZONE_EXTREME;
}

void apply_card_style(lv_obj_t *obj)
{
  lv_obj_set_style_bg_color(obj, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, 12, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 10, 0);
}

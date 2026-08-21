/****************************************************************************
 * apps/examples/fitness/ui_common.h
 *
 * Common definitions for the Middle School Fitness Test LVGL application.
 * Colors, screen dimensions, shared data structures, and page IDs.
 ****************************************************************************/

#ifndef __UI_COMMON_H
#define __UI_COMMON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <lvgl/lvgl.h>
#include <nuttx/config.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Screen dimensions (rectangular AMOLED 390x450) */

#define SCREEN_WIDTH        CONFIG_EXAMPLES_FITNESS_SCREEN_WIDTH
#define SCREEN_HEIGHT       CONFIG_EXAMPLES_FITNESS_SCREEN_HEIGHT

/* Margins and padding */

#define SAFE_MARGIN         12
#define CONTENT_WIDTH       (SCREEN_WIDTH - 2 * SAFE_MARGIN)

/* Color palette (dark theme matching design mockups) */

#define COLOR_BG            lv_color_hex(0x111111)
#define COLOR_CARD_BG       lv_color_hex(0x1E1E2E)
#define COLOR_CARD_BG2      lv_color_hex(0x252535)
#define COLOR_TEXT_PRIMARY   lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_SECONDARY lv_color_hex(0x999999)
#define COLOR_TEXT_DIM       lv_color_hex(0x666666)

/* Accent colors */

#define COLOR_GREEN          lv_color_hex(0x00E676)
#define COLOR_GREEN_DARK     lv_color_hex(0x00C853)
#define COLOR_RED            lv_color_hex(0xFF3B30)
#define COLOR_YELLOW         lv_color_hex(0xFFCC00)
#define COLOR_ORANGE         lv_color_hex(0xFF9500)
#define COLOR_BLUE           lv_color_hex(0x448AFF)

/* Heart rate zone colors */

#define COLOR_HR_REST        lv_color_hex(0x888888)
#define COLOR_HR_FAT_BURN    lv_color_hex(0xFFCC00)
#define COLOR_HR_AEROBIC     lv_color_hex(0x00E676)
#define COLOR_HR_EXTREME     lv_color_hex(0xFF3B30)

/* Progress/bar backgrounds */

#define COLOR_PROGRESS_BG    lv_color_hex(0x333333)
#define COLOR_BORDER         lv_color_hex(0x333333)

/* API server URL */

#define SERVER_URL           CONFIG_EXAMPLES_FITNESS_SERVER_URL
#define DEVICE_SN            CONFIG_EXAMPLES_FITNESS_DEVICE_SN
#define STUDENT_ID           CONFIG_EXAMPLES_FITNESS_STUDENT_ID
#define STUDENT_NAME         CONFIG_EXAMPLES_FITNESS_STUDENT_NAME

/* Fitness test types */

#define TEST_TYPE_JUMP_ROPE     "jump_rope"
#define TEST_TYPE_50M_RUN       "50m_run"
#define TEST_TYPE_LONG_JUMP     "long_jump"
#define TEST_TYPE_SIT_UP        "sit_up"
#define TEST_TYPE_PULL_UP       "pull_up"
#define TEST_TYPE_800M_RUN      "800m_run"
#define TEST_TYPE_1000M_RUN     "1000m_run"

/* Max history records */

#define MAX_HISTORY_RECORDS  20
#define MAX_REPORT_COUNT     5
#define HR_HISTORY_MAX       60

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Page identifiers */

typedef enum
{
  PAGE_SPLASH = 0,
  PAGE_HOME,
  PAGE_SPORT,
  PAGE_RESULT,
  PAGE_DETAIL,
  PAGE_HISTORY,
  PAGE_REPORT,
  PAGE_SETTINGS,
  PAGE_TEST,
  PAGE_COUNT
} page_id_t;

/* Heart rate zone */

typedef enum
{
  HR_ZONE_REST = 0,       /* < 100 bpm */
  HR_ZONE_FAT_BURN,       /* 100-140 bpm */
  HR_ZONE_AEROBIC,        /* 140-170 bpm */
  HR_ZONE_EXTREME,        /* > 170 bpm */
  HR_ZONE_COUNT
} hr_zone_t;

/* Fitness test record */

typedef struct
{
  char     test_type[20];     /* jump_rope, 50m_run, etc. */
  char     test_name[20];     /* Display name */
  int32_t  count;             /* Repetitions or distance*100 */
  float    time_sec;          /* Duration in seconds */
  int32_t  score;             /* 0-100 */
  char     grade[12];         /* excellent/good/pass/fail */
  int32_t  avg_hr;            /* Average heart rate */
  int32_t  max_hr;            /* Max heart rate */
  int32_t  hr_zones[HR_ZONE_COUNT]; /* Zone percentages */
  char     timestamp[20];     /* "YYYY-MM-DD HH:MM" */
} fitness_record_t;

/* Real-time sport data */

typedef struct
{
  int32_t  count;             /* Current count (jump rope, sit-ups, etc.) */
  int32_t  elapsed_sec;       /* Elapsed seconds */
  int32_t  heart_rate;        /* Current heart rate */
  int32_t  max_hr;            /* Max HR during session */
  int32_t  avg_hr;            /* Average HR during session */
  int32_t  hr_sum;            /* Sum for average calculation */
  int32_t  hr_samples;        /* Number of HR samples */
  int32_t  hr_zones[HR_ZONE_COUNT]; /* Zone time in seconds */
  hr_zone_t current_zone;     /* Current HR zone */
  bool     running;           /* Sport session active */
  bool     paused;            /* Sport session paused */
  char     test_type[20];     /* Current test type */
  char     test_name[20];     /* Current test display name */
  int32_t  hr_history[HR_HISTORY_MAX]; /* HR waveform data */
  int32_t  hr_history_idx;    /* Current index in HR ring buffer */
} sport_data_t;

/* Report entry */

typedef struct
{
  char     title[40];         /* Report title */
  char     date[12];          /* "YYYY-MM-DD" */
  char     content[256];      /* Report content */
} report_entry_t;

/* Application settings */

typedef struct
{
  bool     bluetooth_on;      /* Bluetooth pairing enabled */
  bool     wifi_on;           /* WiFi sync enabled */
} app_settings_t;

/* Application global context */

typedef struct
{
  /* Student info */

  char              student_name[32];
  char              student_id[20];
  char              grade_class[20];     /* "Grade 9 Class 2" */

  /* Latest test results */

  int32_t           latest_jump_rope;    /* Latest jump rope count */
  int32_t           latest_avg_hr;       /* Latest average HR */
  int32_t           pass_rate;           /* Overall pass rate % */

  /* Current sport session */

  sport_data_t      sport;

  /* Last completed record */

  fitness_record_t  last_record;

  /* History records */

  fitness_record_t  history[MAX_HISTORY_RECORDS];
  int32_t           history_count;

  /* Reports */

  report_entry_t    reports[MAX_REPORT_COUNT];
  int32_t           report_count;

  /* Device info */

  char              firmware_ver[16];
  bool              imu_ok;
  bool              hr_sensor_ok;
  bool              gps_ok;
  bool              display_ok;

  /* App state */

  app_settings_t    settings;
  page_id_t         current_page;
  bool              network_connected;
  char              server_url[128];
  char              device_sn[32];
  bool              splash_done;
} app_context_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Global context accessor */

app_context_t *app_get_context(void);

/* UI style helpers */

void apply_card_style(lv_obj_t *obj);
void create_nav_header(lv_obj_t *parent, const char *title,
                       void (*back_cb)(lv_event_t *));

/* Grade helpers */

const char *get_grade_text(int32_t score);
lv_color_t get_grade_color(int32_t score);
const char *get_hr_zone_name(hr_zone_t zone);
lv_color_t get_hr_zone_color(hr_zone_t zone);
hr_zone_t classify_hr(int32_t hr);

#endif /* __UI_COMMON_H */

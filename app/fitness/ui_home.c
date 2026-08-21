/****************************************************************************
 * apps/examples/fitness/ui_home.c
 *
 * Home page (今日体测): greeting, 3 metric cards, heart rate waveform,
 * action button, bottom tab navigation.
 *
 * Layout:
 *   - "今日体测" title
 *   - "你好，张小明" greeting card
 *   - 3 metric cards: 最近跳绳 / 平均心率 / 及格率
 *   - Heart rate waveform card
 *   - Green "开始体测" button
 *   - Bottom nav: 运动 / 历史 / 报告 / 设置
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_home.h"
#include "ui_manager.h"
#include "data_collector.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_jump_label;
static lv_obj_t *g_hr_label;
static lv_obj_t *g_rate_label;
static lv_obj_t *g_waveform;

/* Waveform data buffer */

static lv_coord_t g_wave_data[60];
static int g_wave_idx = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void start_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  data_collector_start_sport(TEST_TYPE_JUMP_ROPE);
  ui_manager_switch_page(PAGE_SPORT);
}

/** Create a metric card */

static lv_obj_t *create_metric_card(lv_obj_t *parent, const char *title,
                                     lv_obj_t **value_lbl)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, CONTENT_WIDTH / 3 - 6, 72);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 8, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  /* Title */

  lv_obj_t *t = lv_label_create(card);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_color(t, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 0);

  /* Value */

  *value_lbl = lv_label_create(card);
  lv_label_set_text(*value_lbl, "--");
  lv_obj_set_style_text_color(*value_lbl, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(*value_lbl, &lv_font_montserrat_24, 0);
  lv_obj_align(*value_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);

  return card;
}

/** Create the bottom tab navigation */

static lv_obj_t *create_bottom_nav(lv_obj_t *parent)
{
  lv_obj_t *nav = lv_obj_create(parent);
  lv_obj_set_size(nav, SCREEN_WIDTH, 50);
  lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(nav, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(nav, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(nav, 0, 0);
  lv_obj_set_style_radius(nav, 0, 0);
  lv_obj_set_style_pad_all(nav, 0, 0);
  lv_obj_remove_flag(nav, LV_OBJ_FLAG_SCROLLABLE);

  /* Create 4 tab buttons */

  const char *icons[] = {
    LV_SYMBOL_HOME,
    LV_SYMBOL_LIST,
    LV_SYMBOL_FILE,
    LV_SYMBOL_SETTINGS
  };
  const char *labels[] = {
    "Sport", "History", "Report", "Settings"
  };

  int btn_w = SCREEN_WIDTH / 4;

  for (int i = 0; i < 4; i++)
    {
      lv_obj_t *btn = lv_button_create(nav);
      lv_obj_set_size(btn, btn_w, 46);
      lv_obj_set_pos(btn, i * btn_w, 0);
      lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
      lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
      lv_obj_set_style_radius(btn, 0, 0);
      lv_obj_set_style_border_width(btn, 0, 0);

      lv_obj_t *lbl = lv_label_create(btn);
      lv_label_set_text_fmt(lbl, "%s\n%s", icons[i], labels[i]);
      lv_obj_set_style_text_color(lbl, COLOR_TEXT_SECONDARY, 0);
      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
      lv_obj_center(lbl);
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

      /* Highlight home tab */

      if (i == 0)
        {
          lv_obj_set_style_text_color(lbl, COLOR_GREEN, 0);
        }

      lv_obj_add_event_cb(btn, start_btn_cb, LV_EVENT_CLICKED, NULL);
    }

  return nav;
}

/** Update the waveform chart */

static void update_waveform(void)
{
  /* Add simulated HR waveform data */

  for (int i = 0; i < 60; i++)
    {
      int base = 70;
      int wave = (i * 7 + g_wave_idx * 3) % 30;
      g_wave_data[i] = base + wave;
    }

  g_wave_idx++;

  /* Redraw the waveform as a simple line chart */

  if (g_waveform)
    {
      lv_obj_invalidate(g_waveform);
    }
}

static void waveform_draw_cb(lv_event_t *e)
{
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);

  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);

  int w = coords.x2 - coords.x1;
  int h = coords.y2 - coords.y1;

  /* Draw a polyline representing heart rate waveform */

  lv_point_precise_t points[60];
  for (int i = 0; i < 60; i++)
    {
      points[i].x = (lv_value_precise_t)(coords.x1 + (i * w / 59));
      points[i].y = (lv_value_precise_t)(coords.y2 - (g_wave_data[i] - 50) * h / 80);
    }

  lv_draw_line_dsc_t line_dsc;
  lv_draw_line_dsc_init(&line_dsc);
  line_dsc.color = COLOR_GREEN;
  line_dsc.width = 2;
  line_dsc.round_start = 1;
  line_dsc.round_end = 1;
  line_dsc.p1 = points[0];

  for (int i = 1; i < 60; i++)
    {
      line_dsc.p2 = points[i];
      lv_draw_line(layer, &line_dsc);
      line_dsc.p1 = points[i];
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_home_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  int y_offset = SAFE_MARGIN;

  /* Title: "今日体测" */

  lv_obj_t *title = lv_label_create(page);
  lv_label_set_text(title, "Today's Test");
  lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, y_offset);
  y_offset += 32;

  /* Greeting card */

  lv_obj_t *greet_card = lv_obj_create(page);
  lv_obj_set_size(greet_card, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(greet_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(greet_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(greet_card, 12, 0);
  lv_obj_set_style_border_width(greet_card, 0, 0);
  lv_obj_set_style_pad_all(greet_card, 0, 0);
  lv_obj_remove_flag(greet_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(greet_card, LV_ALIGN_TOP_MID, 0, y_offset);

  lv_obj_t *greet = lv_label_create(greet_card);
  lv_label_set_text(greet, "Hello, " STUDENT_NAME);
  lv_obj_set_style_text_color(greet, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(greet, &lv_font_montserrat_20, 0);
  lv_obj_center(greet);
  y_offset += 56;

  /* 3 metric cards row */

  lv_obj_t *cards_row = lv_obj_create(page);
  lv_obj_set_size(cards_row, CONTENT_WIDTH, 80);
  lv_obj_set_style_bg_opa(cards_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cards_row, 0, 0);
  lv_obj_set_style_pad_all(cards_row, 0, 0);
  lv_obj_set_flex_flow(cards_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cards_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                         LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(cards_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(cards_row, LV_ALIGN_TOP_MID, 0, y_offset);

  create_metric_card(cards_row, "Jump Rope", &g_jump_label);
  create_metric_card(cards_row, "Avg HR", &g_hr_label);
  create_metric_card(cards_row, "Pass %", &g_rate_label);
  y_offset += 92;

  /* Heart rate waveform card */

  lv_obj_t *wave_card = lv_obj_create(page);
  lv_obj_set_size(wave_card, CONTENT_WIDTH, 100);
  lv_obj_set_style_bg_color(wave_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(wave_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(wave_card, 12, 0);
  lv_obj_set_style_border_width(wave_card, 0, 0);
  lv_obj_set_style_pad_all(wave_card, 8, 0);
  lv_obj_remove_flag(wave_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(wave_card, LV_ALIGN_TOP_MID, 0, y_offset);

  lv_obj_t *wave_title = lv_label_create(wave_card);
  lv_label_set_text(wave_title, "Real-time HR");
  lv_obj_set_style_text_color(wave_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(wave_title, &lv_font_montserrat_14, 0);
  lv_obj_align(wave_title, LV_ALIGN_TOP_LEFT, 0, 0);

  /* Waveform drawing area */

  g_waveform = lv_obj_create(wave_card);
  lv_obj_set_size(g_waveform, CONTENT_WIDTH - 16, 60);
  lv_obj_align(g_waveform, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(g_waveform, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_waveform, 0, 0);
  lv_obj_set_style_pad_all(g_waveform, 0, 0);
  lv_obj_remove_flag(g_waveform, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(g_waveform, waveform_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
  y_offset += 112;

  /* Start button */

  lv_obj_t *start_btn = lv_button_create(page);
  lv_obj_set_size(start_btn, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(start_btn, COLOR_GREEN, 0);
  lv_obj_set_style_bg_color(start_btn, COLOR_GREEN_DARK, LV_STATE_PRESSED);
  lv_obj_set_style_radius(start_btn, 22, 0);
  lv_obj_set_style_border_width(start_btn, 0, 0);
  lv_obj_align(start_btn, LV_ALIGN_TOP_MID, 0, y_offset);

  lv_obj_t *btn_lbl = lv_label_create(start_btn);
  lv_label_set_text(btn_lbl, LV_SYMBOL_PLAY " Start Test");
  lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(btn_lbl);
  lv_obj_add_event_cb(start_btn, start_btn_cb, LV_EVENT_CLICKED, NULL);

  /* Bottom navigation */

  create_bottom_nav(page);

  /* Initialize waveform data */

  for (int i = 0; i < 60; i++)
    {
      g_wave_data[i] = 70 + (i * 3) % 20;
    }

  return page;
}

void ui_home_update(void)
{
  app_context_t *ctx = app_get_context();
  char buf[32];

  /* Update metric cards */

  snprintf(buf, sizeof(buf), "%ld", (long)ctx->latest_jump_rope);
  lv_label_set_text(g_jump_label, buf);

  snprintf(buf, sizeof(buf), "%ld", (long)ctx->latest_avg_hr);
  lv_label_set_text(g_hr_label, buf);

  snprintf(buf, sizeof(buf), "%ld%%", (long)ctx->pass_rate);
  lv_label_set_text(g_rate_label, buf);

  /* Update waveform */

  update_waveform();
}

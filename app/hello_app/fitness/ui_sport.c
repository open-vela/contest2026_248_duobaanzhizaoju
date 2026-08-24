/****************************************************************************
 * apps/examples/fitness/ui_sport.c
 *
 * Real-time sport tracking page (实时运动/体测进行中).
 * Shows: jump rope count (large), time, heart rate, start button.
 * Matching design: dark bg, green accent, large numbers.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_sport.h"
#include "ui_manager.h"
#include "data_collector.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_count_label;
static lv_obj_t *g_status_label;
static lv_obj_t *g_time_label;
static lv_obj_t *g_hr_label;
static lv_obj_t *g_start_btn;
static lv_obj_t *g_start_lbl;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void back_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  data_collector_stop_sport();
  ui_manager_switch_page(PAGE_HOME);
}

static void start_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  app_context_t *ctx = app_get_context();

  if (!ctx->sport.running)
    {
      /* Start the sport session */

      data_collector_start_sport(TEST_TYPE_JUMP_ROPE);
      lv_label_set_text(g_start_lbl, "Stop");
      lv_obj_set_style_bg_color(g_start_btn, COLOR_RED, 0);
    }
  else
    {
      /* Stop and go to results */

      data_collector_stop_sport();

      /* Build the last record from sport data */

      const sport_data_t *sp = &ctx->sport;
      fitness_record_t *rec = &ctx->last_record;
      memset(rec, 0, sizeof(*rec));
      strncpy(rec->test_type, sp->test_type, sizeof(rec->test_type) - 1);
      strncpy(rec->test_name, sp->test_name, sizeof(rec->test_name) - 1);
      rec->count = sp->count;
      rec->time_sec = (float)sp->elapsed_sec;
      rec->avg_hr = sp->avg_hr;
      rec->max_hr = sp->max_hr;
      memcpy(rec->hr_zones, sp->hr_zones, sizeof(sp->hr_zones));
      rec->score = calculate_score(sp->test_type, sp->count,
                                    (float)sp->elapsed_sec);

      const char *g = get_grade_text(rec->score);
      strncpy(rec->grade, g, sizeof(rec->grade) - 1);

      ui_manager_switch_page(PAGE_RESULT);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_sport_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  /* Back button + title */

  lv_obj_t *header = lv_obj_create(page);
  lv_obj_set_size(header, CONTENT_WIDTH, 36);
  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, SAFE_MARGIN);

  lv_obj_t *back = lv_button_create(header);
  lv_obj_set_size(back, 36, 36);
  lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(back, 0, 0);
  lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_t *back_lbl = lv_label_create(back);
  lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(back_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_center(back_lbl);
  lv_obj_add_event_cb(back, back_btn_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *header_title = lv_label_create(header);
  lv_label_set_text(header_title, "Test In Progress");
  lv_obj_set_style_text_color(header_title, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(header_title, &lv_font_montserrat_16, 0);
  lv_obj_align(header_title, LV_ALIGN_LEFT_MID, 44, 0);

  /* Main count card */

  lv_obj_t *count_card = lv_obj_create(page);
  lv_obj_set_size(count_card, CONTENT_WIDTH, 140);
  lv_obj_set_style_bg_color(count_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(count_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(count_card, 16, 0);
  lv_obj_set_style_border_width(count_card, 0, 0);
  lv_obj_set_style_pad_all(count_card, 12, 0);
  lv_obj_remove_flag(count_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(count_card, LV_ALIGN_TOP_MID, 0, 56);

  lv_obj_t *count_title = lv_label_create(count_card);
  lv_label_set_text(count_title, "Jump Rope");
  lv_obj_set_style_text_color(count_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(count_title, &lv_font_montserrat_16, 0);
  lv_obj_align(count_title, LV_ALIGN_TOP_MID, 0, 0);

  g_count_label = lv_label_create(count_card);
  lv_label_set_text(g_count_label, "0");
  lv_obj_set_style_text_color(g_count_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_count_label, &lv_font_montserrat_36, 0);
  lv_obj_align(g_count_label, LV_ALIGN_CENTER, 0, 5);

  g_status_label = lv_label_create(count_card);
  lv_label_set_text(g_status_label, "Idle");
  lv_obj_set_style_text_color(g_status_label, COLOR_TEXT_DIM, 0);
  lv_obj_set_style_text_font(g_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(g_status_label, LV_ALIGN_BOTTOM_MID, 0, 0);

  /* Time and Heart rate cards side by side */

  lv_obj_t *metrics_row = lv_obj_create(page);
  lv_obj_set_size(metrics_row, CONTENT_WIDTH, 80);
  lv_obj_set_style_bg_opa(metrics_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(metrics_row, 0, 0);
  lv_obj_set_style_pad_all(metrics_row, 0, 0);
  lv_obj_set_flex_flow(metrics_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(metrics_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                         LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(metrics_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(metrics_row, LV_ALIGN_TOP_MID, 0, 210);

  /* Time card */

  lv_obj_t *time_card = lv_obj_create(metrics_row);
  lv_obj_set_size(time_card, CONTENT_WIDTH / 2 - 6, 72);
  lv_obj_set_style_bg_color(time_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(time_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(time_card, 12, 0);
  lv_obj_set_style_border_width(time_card, 0, 0);
  lv_obj_set_style_pad_all(time_card, 8, 0);
  lv_obj_remove_flag(time_card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *time_title = lv_label_create(time_card);
  lv_label_set_text(time_title, "Time");
  lv_obj_set_style_text_color(time_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(time_title, &lv_font_montserrat_14, 0);
  lv_obj_align(time_title, LV_ALIGN_TOP_MID, 0, 0);

  g_time_label = lv_label_create(time_card);
  lv_label_set_text(g_time_label, "00:00");
  lv_obj_set_style_text_color(g_time_label, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_time_label, &lv_font_montserrat_28, 0);
  lv_obj_align(g_time_label, LV_ALIGN_BOTTOM_MID, 0, 0);

  /* Heart rate card */

  lv_obj_t *hr_card = lv_obj_create(metrics_row);
  lv_obj_set_size(hr_card, CONTENT_WIDTH / 2 - 6, 72);
  lv_obj_set_style_bg_color(hr_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(hr_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(hr_card, 12, 0);
  lv_obj_set_style_border_width(hr_card, 0, 0);
  lv_obj_set_style_pad_all(hr_card, 8, 0);
  lv_obj_remove_flag(hr_card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *hr_title = lv_label_create(hr_card);
  lv_label_set_text(hr_title, "Heart Rate");
  lv_obj_set_style_text_color(hr_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(hr_title, &lv_font_montserrat_14, 0);
  lv_obj_align(hr_title, LV_ALIGN_TOP_MID, 0, 0);

  g_hr_label = lv_label_create(hr_card);
  lv_label_set_text(g_hr_label, "0");
  lv_obj_set_style_text_color(g_hr_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_hr_label, &lv_font_montserrat_28, 0);
  lv_obj_align(g_hr_label, LV_ALIGN_BOTTOM_MID, 0, 0);

  /* Start/Stop button */

  g_start_btn = lv_button_create(page);
  lv_obj_set_size(g_start_btn, CONTENT_WIDTH, 48);
  lv_obj_set_style_bg_color(g_start_btn, COLOR_GREEN, 0);
  lv_obj_set_style_bg_color(g_start_btn, COLOR_GREEN_DARK, LV_STATE_PRESSED);
  lv_obj_set_style_radius(g_start_btn, 24, 0);
  lv_obj_set_style_border_width(g_start_btn, 0, 0);
  lv_obj_align(g_start_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

  g_start_lbl = lv_label_create(g_start_btn);
  lv_label_set_text(g_start_lbl, LV_SYMBOL_PLAY " Start");
  lv_obj_set_style_text_color(g_start_lbl, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(g_start_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(g_start_lbl);
  lv_obj_add_event_cb(g_start_btn, start_btn_cb, LV_EVENT_CLICKED, NULL);

  return page;
}

void ui_sport_update(void)
{
  const sport_data_t *sp = data_collector_get_sport_data();
  char buf[32];

  /* Update count */

  snprintf(buf, sizeof(buf), "%ld", (long)sp->count);
  lv_label_set_text(g_count_label, buf);

  /* Update status */

  if (sp->running && !sp->paused)
    {
      lv_label_set_text(g_status_label, "Exercising");
      lv_obj_set_style_text_color(g_status_label, COLOR_GREEN, 0);
    }
  else if (sp->paused)
    {
      lv_label_set_text(g_status_label, "Paused");
      lv_obj_set_style_text_color(g_status_label, COLOR_YELLOW, 0);
    }
  else
    {
      lv_label_set_text(g_status_label, "Idle");
      lv_obj_set_style_text_color(g_status_label, COLOR_TEXT_DIM, 0);
    }

  /* Update time */

  int min = sp->elapsed_sec / 60;
  int sec = sp->elapsed_sec % 60;
  snprintf(buf, sizeof(buf), "%02d:%02d", min, sec);
  lv_label_set_text(g_time_label, buf);

  /* Update heart rate */

  snprintf(buf, sizeof(buf), "%ld", (long)sp->heart_rate);
  lv_label_set_text(g_hr_label, buf);

  if (sp->heart_rate > 170)
    {
      lv_obj_set_style_text_color(g_hr_label, COLOR_RED, 0);
    }
  else if (sp->heart_rate > 140)
    {
      lv_obj_set_style_text_color(g_hr_label, COLOR_ORANGE, 0);
    }
  else
    {
      lv_obj_set_style_text_color(g_hr_label, COLOR_GREEN, 0);
    }
}

void ui_sport_reset(void)
{
  lv_label_set_text(g_count_label, "0");
  lv_label_set_text(g_time_label, "00:00");
  lv_label_set_text(g_hr_label, "0");
  lv_label_set_text(g_status_label, "Idle");
  lv_label_set_text(g_start_lbl, LV_SYMBOL_PLAY " Start");
  lv_obj_set_style_bg_color(g_start_btn, COLOR_GREEN, 0);
}

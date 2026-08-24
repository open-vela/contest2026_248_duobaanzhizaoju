/****************************************************************************
 * apps/examples/fitness/ui_result.c
 *
 * Score result page (成绩结果): overall score, grade, metrics,
 * "View Details" and "Return Home" buttons.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_result.h"
#include "ui_manager.h"
#include "ui_sport.h"
#include "data_collector.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_score_label;
static lv_obj_t *g_grade_label;
static lv_obj_t *g_count_val;
static lv_obj_t *g_time_val;
static lv_obj_t *g_avg_hr_val;
static lv_obj_t *g_max_hr_val;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void detail_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_DETAIL);
}

static void home_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_sport_reset();
  ui_manager_switch_page(PAGE_HOME);
}

/** Create a small metric card in the 4-card row */

static lv_obj_t *create_result_metric(lv_obj_t *parent, const char *title,
                                       lv_obj_t **val_lbl, lv_color_t val_color)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, CONTENT_WIDTH / 4 - 6, 68);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 4, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_color(t, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 2);

  *val_lbl = lv_label_create(card);
  lv_label_set_text(*val_lbl, "--");
  lv_obj_set_style_text_color(*val_lbl, val_color, 0);
  lv_obj_set_style_text_font(*val_lbl, &lv_font_montserrat_16, 0);
  lv_obj_align(*val_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);

  return card;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_result_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  /* Header: back + title */

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
  lv_obj_add_event_cb(back, home_btn_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *ht = lv_label_create(header);
  lv_label_set_text(ht, "Score Result");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_16, 0);
  lv_obj_align(ht, LV_ALIGN_LEFT_MID, 44, 0);

  /* Main score card */

  lv_obj_t *score_card = lv_obj_create(page);
  lv_obj_set_size(score_card, CONTENT_WIDTH, 120);
  lv_obj_set_style_bg_color(score_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(score_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(score_card, 16, 0);
  lv_obj_set_style_border_width(score_card, 0, 0);
  lv_obj_set_style_pad_all(score_card, 0, 0);
  lv_obj_remove_flag(score_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(score_card, LV_ALIGN_TOP_MID, 0, 56);

  g_score_label = lv_label_create(score_card);
  lv_label_set_text(g_score_label, "90");
  lv_obj_set_style_text_color(g_score_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_score_label, &lv_font_montserrat_36, 0);
  lv_obj_align(g_score_label, LV_ALIGN_CENTER, 0, -12);

  g_grade_label = lv_label_create(score_card);
  lv_label_set_text(g_grade_label, "Good");
  lv_obj_set_style_text_color(g_grade_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_grade_label, &lv_font_montserrat_20, 0);
  lv_obj_align(g_grade_label, LV_ALIGN_CENTER, 0, 22);

  /* 4 metric cards row */

  lv_obj_t *metrics_row = lv_obj_create(page);
  lv_obj_set_size(metrics_row, CONTENT_WIDTH, 76);
  lv_obj_set_style_bg_opa(metrics_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(metrics_row, 0, 0);
  lv_obj_set_style_pad_all(metrics_row, 0, 0);
  lv_obj_set_flex_flow(metrics_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(metrics_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                         LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(metrics_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(metrics_row, LV_ALIGN_TOP_MID, 0, 190);

  create_result_metric(metrics_row, "Count", &g_count_val, COLOR_TEXT_PRIMARY);
  create_result_metric(metrics_row, "Time", &g_time_val, COLOR_TEXT_PRIMARY);
  create_result_metric(metrics_row, "Avg HR", &g_avg_hr_val, COLOR_GREEN);
  create_result_metric(metrics_row, "Max HR", &g_max_hr_val, COLOR_RED);

  /* View Details button */

  lv_obj_t *detail_btn = lv_button_create(page);
  lv_obj_set_size(detail_btn, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(detail_btn, COLOR_GREEN, 0);
  lv_obj_set_style_radius(detail_btn, 22, 0);
  lv_obj_set_style_border_width(detail_btn, 0, 0);
  lv_obj_align(detail_btn, LV_ALIGN_TOP_MID, 0, 286);

  lv_obj_t *db_lbl = lv_label_create(detail_btn);
  lv_label_set_text(db_lbl, "View Details");
  lv_obj_set_style_text_color(db_lbl, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(db_lbl, &lv_font_montserrat_16, 0);
  lv_obj_center(db_lbl);
  lv_obj_add_event_cb(detail_btn, detail_btn_cb, LV_EVENT_CLICKED, NULL);

  /* Return Home button */

  lv_obj_t *home_btn = lv_button_create(page);
  lv_obj_set_size(home_btn, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(home_btn, COLOR_CARD_BG2, 0);
  lv_obj_set_style_radius(home_btn, 22, 0);
  lv_obj_set_style_border_width(home_btn, 0, 0);
  lv_obj_align(home_btn, LV_ALIGN_TOP_MID, 0, 340);

  lv_obj_t *hb_lbl = lv_label_create(home_btn);
  lv_label_set_text(hb_lbl, "Return Home");
  lv_obj_set_style_text_color(hb_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(hb_lbl, &lv_font_montserrat_16, 0);
  lv_obj_center(hb_lbl);
  lv_obj_add_event_cb(home_btn, home_btn_cb, LV_EVENT_CLICKED, NULL);

  return page;
}

void ui_result_update(void)
{
  app_context_t *ctx = app_get_context();
  fitness_record_t *rec = &ctx->last_record;
  char buf[32];

  /* Update score */

  snprintf(buf, sizeof(buf), "%ld", (long)rec->score);
  lv_label_set_text(g_score_label, buf);
  lv_obj_set_style_text_color(g_score_label, get_grade_color(rec->score), 0);

  /* Update grade */

  lv_label_set_text(g_grade_label, get_grade_text(rec->score));
  lv_obj_set_style_text_color(g_grade_label, get_grade_color(rec->score), 0);

  /* Update metrics */

  snprintf(buf, sizeof(buf), "%ld", (long)rec->count);
  lv_label_set_text(g_count_val, buf);

  int min = (int)rec->time_sec / 60;
  int sec = (int)rec->time_sec % 60;
  snprintf(buf, sizeof(buf), "%d''", sec);
  lv_label_set_text(g_time_val, buf);

  snprintf(buf, sizeof(buf), "%ld", (long)rec->avg_hr);
  lv_label_set_text(g_avg_hr_val, buf);

  snprintf(buf, sizeof(buf), "%ld", (long)rec->max_hr);
  lv_label_set_text(g_max_hr_val, buf);
}

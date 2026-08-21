/****************************************************************************
 * apps/examples/fitness/ui_detail.c
 *
 * Fitness test detail page (体测详情): score, HR zones, test info.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_detail.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_test_name;
static lv_obj_t *g_count_label;
static lv_obj_t *g_grade_label;
static lv_obj_t *g_score_val;
static lv_obj_t *g_avg_hr_val;
static lv_obj_t *g_max_hr_val;
static lv_obj_t *g_zone_bars[HR_ZONE_COUNT];
static lv_obj_t *g_zone_pcts[HR_ZONE_COUNT];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void back_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_RESULT);
}

/** Create a metric info card */

static lv_obj_t *create_info_card(lv_obj_t *parent, const char *title,
                                    lv_obj_t **val_lbl)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, CONTENT_WIDTH / 3 - 6, 64);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 6, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_color(t, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 0);

  *val_lbl = lv_label_create(card);
  lv_label_set_text(*val_lbl, "--");
  lv_obj_set_style_text_color(*val_lbl, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(*val_lbl, &lv_font_montserrat_20, 0);
  lv_obj_align(*val_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);

  return card;
}

/** Create a heart rate zone bar */

static void create_zone_row(lv_obj_t *parent, const char *name,
                              lv_color_t color, int idx)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, CONTENT_WIDTH - 16, 28);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  /* Zone name */

  lv_obj_t *name_lbl = lv_label_create(row);
  lv_label_set_text(name_lbl, name);
  lv_obj_set_style_text_color(name_lbl, color, 0);
  lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 0, 0);

  /* Progress bar */

  g_zone_bars[idx] = lv_bar_create(row);
  lv_obj_set_size(g_zone_bars[idx], 140, 10);
  lv_obj_align(g_zone_bars[idx], LV_ALIGN_LEFT_MID, 60, 0);
  lv_bar_set_range(g_zone_bars[idx], 0, 100);
  lv_bar_set_value(g_zone_bars[idx], 0, LV_ANIM_OFF);
  lv_obj_set_style_radius(g_zone_bars[idx], 5, 0);
  lv_obj_set_style_radius(g_zone_bars[idx], 5, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(g_zone_bars[idx], COLOR_PROGRESS_BG, 0);
  lv_obj_set_style_bg_color(g_zone_bars[idx], color, LV_PART_INDICATOR);

  /* Percentage label */

  g_zone_pcts[idx] = lv_label_create(row);
  lv_label_set_text(g_zone_pcts[idx], "0%");
  lv_obj_set_style_text_color(g_zone_pcts[idx], COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(g_zone_pcts[idx], &lv_font_montserrat_14, 0);
  lv_obj_align(g_zone_pcts[idx], LV_ALIGN_RIGHT_MID, 0, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_detail_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  /* Header */

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

  lv_obj_t *ht = lv_label_create(header);
  lv_label_set_text(ht, "Test Details");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_16, 0);
  lv_obj_align(ht, LV_ALIGN_LEFT_MID, 44, 0);

  /* Main info card: test name, count, grade */

  lv_obj_t *info_card = lv_obj_create(page);
  lv_obj_set_size(info_card, CONTENT_WIDTH, 80);
  lv_obj_set_style_bg_color(info_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(info_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(info_card, 14, 0);
  lv_obj_set_style_border_width(info_card, 0, 0);
  lv_obj_set_style_pad_all(info_card, 12, 0);
  lv_obj_remove_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(info_card, LV_ALIGN_TOP_MID, 0, 56);

  g_test_name = lv_label_create(info_card);
  lv_label_set_text(g_test_name, "Jump Rope");
  lv_obj_set_style_text_color(g_test_name, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_test_name, &lv_font_montserrat_20, 0);
  lv_obj_align(g_test_name, LV_ALIGN_TOP_LEFT, 0, 0);

  g_count_label = lv_label_create(info_card);
  lv_label_set_text(g_count_label, "175");
  lv_obj_set_style_text_color(g_count_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_count_label, &lv_font_montserrat_24, 0);
  lv_obj_align(g_count_label, LV_ALIGN_TOP_LEFT, 100, 0);

  g_grade_label = lv_label_create(info_card);
  lv_label_set_text(g_grade_label, "Good");
  lv_obj_set_style_text_color(g_grade_label, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(g_grade_label, &lv_font_montserrat_16, 0);
  lv_obj_align(g_grade_label, LV_ALIGN_TOP_RIGHT, 0, 4);

  /* 3 info metric cards */

  lv_obj_t *info_row = lv_obj_create(page);
  lv_obj_set_size(info_row, CONTENT_WIDTH, 72);
  lv_obj_set_style_bg_opa(info_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(info_row, 0, 0);
  lv_obj_set_style_pad_all(info_row, 0, 0);
  lv_obj_set_flex_flow(info_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(info_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                         LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(info_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(info_row, LV_ALIGN_TOP_MID, 0, 148);

  create_info_card(info_row, "Score", &g_score_val);
  create_info_card(info_row, "Avg HR", &g_avg_hr_val);
  create_info_card(info_row, "Max HR", &g_max_hr_val);

  /* Heart rate zone section */

  lv_obj_t *zone_title = lv_label_create(page);
  lv_label_set_text(zone_title, "HR Zone Distribution");
  lv_obj_set_style_text_color(zone_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(zone_title, &lv_font_montserrat_14, 0);
  lv_obj_align(zone_title, LV_ALIGN_TOP_LEFT, SAFE_MARGIN, 232);

  lv_obj_t *zone_cont = lv_obj_create(page);
  lv_obj_set_size(zone_cont, CONTENT_WIDTH, 140);
  lv_obj_set_style_bg_color(zone_cont, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(zone_cont, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(zone_cont, 12, 0);
  lv_obj_set_style_border_width(zone_cont, 0, 0);
  lv_obj_set_style_pad_all(zone_cont, 8, 0);
  lv_obj_set_flex_flow(zone_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(zone_cont, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(zone_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(zone_cont, LV_ALIGN_TOP_MID, 0, 250);

  create_zone_row(zone_cont, "Rest", COLOR_HR_REST, HR_ZONE_REST);
  create_zone_row(zone_cont, "Fat Burn", COLOR_HR_FAT_BURN, HR_ZONE_FAT_BURN);
  create_zone_row(zone_cont, "Aerobic", COLOR_HR_AEROBIC, HR_ZONE_AEROBIC);
  create_zone_row(zone_cont, "Extreme", COLOR_HR_EXTREME, HR_ZONE_EXTREME);

  return page;
}

void ui_detail_update(void)
{
  app_context_t *ctx = app_get_context();
  fitness_record_t *rec = &ctx->last_record;
  char buf[32];

  /* Update test info */

  lv_label_set_text(g_test_name, rec->test_name);

  snprintf(buf, sizeof(buf), "%ld", (long)rec->count);
  lv_label_set_text(g_count_label, buf);

  lv_label_set_text(g_grade_label, get_grade_text(rec->score));
  lv_obj_set_style_text_color(g_grade_label, get_grade_color(rec->score), 0);

  /* Update info cards */

  snprintf(buf, sizeof(buf), "%ld", (long)rec->score);
  lv_label_set_text(g_score_val, buf);
  lv_obj_set_style_text_color(g_score_val, get_grade_color(rec->score), 0);

  snprintf(buf, sizeof(buf), "%ld", (long)rec->avg_hr);
  lv_label_set_text(g_avg_hr_val, buf);

  snprintf(buf, sizeof(buf), "%ld", (long)rec->max_hr);
  lv_label_set_text(g_max_hr_val, buf);

  /* Update zone bars */

  const char *zone_names[] = {"Rest", "Fat Burn", "Aerobic", "Extreme"};
  for (int i = 0; i < HR_ZONE_COUNT; i++)
    {
      lv_bar_set_value(g_zone_bars[i], rec->hr_zones[i], LV_ANIM_ON);
      snprintf(buf, sizeof(buf), "%ld%%", (long)rec->hr_zones[i]);
      lv_label_set_text(g_zone_pcts[i], buf);
    }
}

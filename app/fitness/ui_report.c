/****************************************************************************
 * apps/examples/fitness/ui_report.c
 *
 * Report page: AI-generated report list and report detail view.
 * Shows: 体质健康综合报告, 月度趋势报告, 薄弱项提升建议
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_report.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_list_cont;
static lv_obj_t *g_detail_panel;
static lv_obj_t *g_detail_title;
static lv_obj_t *g_detail_content;
static bool g_showing_detail = false;

/* Navigation callback */

static void tab_btn_cb(lv_event_t *e)
{
  page_id_t page = (page_id_t)(uintptr_t)lv_event_get_user_data(e);
  ui_manager_switch_page(page);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void report_item_cb(lv_event_t *e)
{
  int idx = (int)(uintptr_t)lv_event_get_user_data(e);
  app_context_t *ctx = app_get_context();

  if (idx < ctx->report_count)
    {
      lv_label_set_text(g_detail_title, ctx->reports[idx].title);
      lv_label_set_text(g_detail_content, ctx->reports[idx].content);
      lv_obj_remove_flag(g_detail_panel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(g_list_cont, LV_OBJ_FLAG_HIDDEN);
      g_showing_detail = true;
    }
}

static void back_from_detail_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_obj_add_flag(g_detail_panel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(g_list_cont, LV_OBJ_FLAG_HIDDEN);
  g_showing_detail = false;
}

static void back_to_home_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  if (g_showing_detail)
    {
      lv_obj_add_flag(g_detail_panel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(g_list_cont, LV_OBJ_FLAG_HIDDEN);
      g_showing_detail = false;
    }
  else
    {
      ui_manager_switch_page(PAGE_HOME);
    }
}

/** Create a report list item */

static void create_report_item(lv_obj_t *parent, const char *title,
                                 const char *date, int idx)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, CONTENT_WIDTH, 64);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *t = lv_label_create(card);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_color(t, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_16, 0);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *d = lv_label_create(card);
  lv_label_set_text(d, date);
  lv_obj_set_style_text_color(d, COLOR_TEXT_DIM, 0);
  lv_obj_set_style_text_font(d, &lv_font_montserrat_14, 0);
  lv_obj_align(d, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  lv_obj_add_event_cb(card, report_item_cb, LV_EVENT_CLICKED,
                       (void *)(uintptr_t)idx);
}

/** Create bottom navigation */

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

  const char *icons[] = {
    LV_SYMBOL_HOME, LV_SYMBOL_LIST,
    LV_SYMBOL_FILE, LV_SYMBOL_SETTINGS
  };
  const char *labels[] = {
    "Sport", "History", "Report", "Settings"
  };
  page_id_t pages[] = {
    PAGE_HOME, PAGE_HISTORY, PAGE_REPORT, PAGE_SETTINGS
  };

  int btn_w = SCREEN_WIDTH / 4;

  for (int i = 0; i < 4; i++)
    {
      lv_obj_t *btn = lv_button_create(nav);
      lv_obj_set_size(btn, btn_w, 46);
      lv_obj_set_pos(btn, i * btn_w, 0);
      lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
      lv_obj_set_style_radius(btn, 0, 0);
      lv_obj_set_style_border_width(btn, 0, 0);

      lv_obj_t *lbl = lv_label_create(btn);
      lv_label_set_text_fmt(lbl, "%s\n%s", icons[i], labels[i]);
      lv_obj_set_style_text_color(lbl, COLOR_TEXT_SECONDARY, 0);
      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
      lv_obj_center(lbl);
      lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

      if (i == 2)
        {
          lv_obj_set_style_text_color(lbl, COLOR_GREEN, 0);
        }

      lv_obj_add_event_cb(btn, tab_btn_cb, LV_EVENT_CLICKED,
                           (void *)(uintptr_t)pages[i]);
    }

  return nav;
}

/** Populate reports with demo data */

static void populate_reports(lv_obj_t *cont)
{
  app_context_t *ctx = app_get_context();

  /* Initialize demo reports if empty */

  if (ctx->report_count == 0)
    {
      ctx->report_count = 3;

      strncpy(ctx->reports[0].title, "Physical Health Report",
              sizeof(ctx->reports[0].title) - 1);
      strncpy(ctx->reports[0].date, "2026-07-08",
              sizeof(ctx->reports[0].date) - 1);
      strncpy(ctx->reports[0].content,
              "Overall Score: 87/100 - Good\n\n"
              "Strengths:\n- Jump Rope: 175 times (Good)\n\n"
              "Weak Areas:\n- Pull-ups: 12 reps (Pass)\n\n"
              "Suggestions:\n"
              "1. Upper body strength training 2x/week\n"
              "2. Sprint start reaction drills\n"
              "3. Maintain daily jump rope practice",
              sizeof(ctx->reports[0].content) - 1);

      strncpy(ctx->reports[1].title, "Monthly Trend Report",
              sizeof(ctx->reports[1].title) - 1);
      strncpy(ctx->reports[1].date, "2026-06-30",
              sizeof(ctx->reports[1].date) - 1);
      strncpy(ctx->reports[1].content,
              "Monthly Progress Summary:\n\n"
              "Jump Rope: 140 -> 160 -> 175 (improving)\n"
              "50m Run: 7.8s -> 7.5s -> 7.3s (improving)\n"
              "Sit-ups: 42 -> 45 -> 48 (improving)\n\n"
              "Overall trend: Positive improvement across all items.",
              sizeof(ctx->reports[1].content) - 1);

      strncpy(ctx->reports[2].title, "Weak Area Suggestions",
              sizeof(ctx->reports[2].title) - 1);
      strncpy(ctx->reports[2].date, "2026-06-15",
              sizeof(ctx->reports[2].date) - 1);
      strncpy(ctx->reports[2].content,
              "Based on analysis of your test results:\n\n"
              "1. Pull-ups: Current 12 reps, target 15+\n"
              "   - Add lat pulldowns and inverted rows\n"
              "   - Practice dead hangs for grip strength\n\n"
              "2. 50m Sprint: Current 7.3s, target 7.0s\n"
              "   - Improve start reaction time\n"
              "   - Add interval sprints to training",
              sizeof(ctx->reports[2].content) - 1);
    }

  /* Section title */

  lv_obj_t *sec = lv_label_create(cont);
  lv_label_set_text(sec, "AI Generated Reports");
  lv_obj_set_style_text_color(sec, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sec, &lv_font_montserrat_14, 0);
  lv_obj_set_width(sec, CONTENT_WIDTH);

  /* Report items */

  for (int i = 0; i < ctx->report_count; i++)
    {
      create_report_item(cont, ctx->reports[i].title,
                          ctx->reports[i].date, i);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_report_create(lv_obj_t *parent)
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

  lv_obj_t *ht = lv_label_create(header);
  lv_label_set_text(ht, "Fitness Reports");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_20, 0);
  lv_obj_align(ht, LV_ALIGN_CENTER, 0, 0);

  /* Report list container */

  g_list_cont = lv_obj_create(page);
  lv_obj_set_size(g_list_cont, CONTENT_WIDTH, SCREEN_HEIGHT - 120);
  lv_obj_set_style_bg_opa(g_list_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_list_cont, 0, 0);
  lv_obj_set_style_pad_all(g_list_cont, 0, 0);
  lv_obj_set_style_pad_row(g_list_cont, 8, 0);
  lv_obj_set_flex_flow(g_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_list_cont, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align(g_list_cont, LV_ALIGN_TOP_MID, 0, 52);

  populate_reports(g_list_cont);

  /* Detail panel (hidden initially) */

  g_detail_panel = lv_obj_create(page);
  lv_obj_set_size(g_detail_panel, CONTENT_WIDTH, SCREEN_HEIGHT - 70);
  lv_obj_set_style_bg_color(g_detail_panel, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(g_detail_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g_detail_panel, 0, 0);
  lv_obj_set_style_pad_all(g_detail_panel, 12, 0);
  lv_obj_align(g_detail_panel, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_add_flag(g_detail_panel, LV_OBJ_FLAG_HIDDEN);

  /* Detail back button */

  lv_obj_t *det_back = lv_button_create(g_detail_panel);
  lv_obj_set_size(det_back, 36, 36);
  lv_obj_set_style_bg_opa(det_back, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(det_back, 0, 0);
  lv_obj_align(det_back, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_t *det_back_lbl = lv_label_create(det_back);
  lv_label_set_text(det_back_lbl, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(det_back_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_center(det_back_lbl);
  lv_obj_add_event_cb(det_back, back_from_detail_cb, LV_EVENT_CLICKED, NULL);

  /* Detail title */

  g_detail_title = lv_label_create(g_detail_panel);
  lv_label_set_text(g_detail_title, "Report Detail");
  lv_obj_set_style_text_color(g_detail_title, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_detail_title, &lv_font_montserrat_20, 0);
  lv_obj_align(g_detail_title, LV_ALIGN_TOP_MID, 0, 8);

  /* Score display */

  lv_obj_t *score_card = lv_obj_create(g_detail_panel);
  lv_obj_set_size(score_card, CONTENT_WIDTH - 24, 80);
  lv_obj_set_style_bg_color(score_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(score_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(score_card, 14, 0);
  lv_obj_set_style_border_width(score_card, 0, 0);
  lv_obj_remove_flag(score_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(score_card, LV_ALIGN_TOP_MID, 0, 50);

  lv_obj_t *score_num = lv_label_create(score_card);
  lv_label_set_text(score_num, "87");
  lv_obj_set_style_text_color(score_num, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(score_num, &lv_font_montserrat_36, 0);
  lv_obj_align(score_num, LV_ALIGN_CENTER, 0, -8);

  lv_obj_t *score_desc = lv_label_create(score_card);
  lv_label_set_text(score_desc, "Good - Overall Score");
  lv_obj_set_style_text_color(score_desc, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(score_desc, &lv_font_montserrat_14, 0);
  lv_obj_align(score_desc, LV_ALIGN_CENTER, 0, 20);

  /* Strengths section */

  lv_obj_t *str_title = lv_label_create(g_detail_panel);
  lv_label_set_text(str_title, "Strengths");
  lv_obj_set_style_text_color(str_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(str_title, &lv_font_montserrat_14, 0);
  lv_obj_align(str_title, LV_ALIGN_TOP_LEFT, 0, 144);

  lv_obj_t *str_card = lv_obj_create(g_detail_panel);
  lv_obj_set_size(str_card, CONTENT_WIDTH - 24, 36);
  lv_obj_set_style_bg_color(str_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(str_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(str_card, 8, 0);
  lv_obj_set_style_border_width(str_card, 0, 0);
  lv_obj_remove_flag(str_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(str_card, LV_ALIGN_TOP_LEFT, 0, 162);

  lv_obj_t *str_val = lv_label_create(str_card);
  lv_label_set_text(str_val, "Jump Rope 175 (Good)");
  lv_obj_set_style_text_color(str_val, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(str_val, &lv_font_montserrat_14, 0);
  lv_obj_center(str_val);

  /* Weak areas section */

  lv_obj_t *weak_title = lv_label_create(g_detail_panel);
  lv_label_set_text(weak_title, "Weak Areas");
  lv_obj_set_style_text_color(weak_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(weak_title, &lv_font_montserrat_14, 0);
  lv_obj_align(weak_title, LV_ALIGN_TOP_LEFT, 0, 208);

  lv_obj_t *weak_card = lv_obj_create(g_detail_panel);
  lv_obj_set_size(weak_card, CONTENT_WIDTH - 24, 36);
  lv_obj_set_style_bg_color(weak_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(weak_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(weak_card, 8, 0);
  lv_obj_set_style_border_width(weak_card, 0, 0);
  lv_obj_remove_flag(weak_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(weak_card, LV_ALIGN_TOP_LEFT, 0, 226);

  lv_obj_t *weak_val = lv_label_create(weak_card);
  lv_label_set_text(weak_val, "Pull-ups 12 (Pass)");
  lv_obj_set_style_text_color(weak_val, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(weak_val, &lv_font_montserrat_14, 0);
  lv_obj_center(weak_val);

  /* Suggestions section */

  lv_obj_t *sug_title = lv_label_create(g_detail_panel);
  lv_label_set_text(sug_title, "Suggestions");
  lv_obj_set_style_text_color(sug_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sug_title, &lv_font_montserrat_14, 0);
  lv_obj_align(sug_title, LV_ALIGN_TOP_LEFT, 0, 272);

  lv_obj_t *sug_card = lv_obj_create(g_detail_panel);
  lv_obj_set_size(sug_card, CONTENT_WIDTH - 24, 100);
  lv_obj_set_style_bg_color(sug_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(sug_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(sug_card, 8, 0);
  lv_obj_set_style_border_width(sug_card, 0, 0);
  lv_obj_remove_flag(sug_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(sug_card, LV_ALIGN_TOP_LEFT, 0, 290);

  g_detail_content = lv_label_create(sug_card);
  lv_label_set_text(g_detail_content,
                     "1. Upper body 2x/week\n"
                     "2. Sprint start drills\n"
                     "3. Daily jump rope");
  lv_obj_set_style_text_color(g_detail_content, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_detail_content, &lv_font_montserrat_14, 0);
  lv_obj_align(g_detail_content, LV_ALIGN_TOP_LEFT, 4, 4);

  /* Bottom navigation */

  create_bottom_nav(page);

  return page;
}

void ui_report_update(void)
{
  /* Could refresh report data here */
}

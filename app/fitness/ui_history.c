/****************************************************************************
 * apps/examples/fitness/ui_history.c
 *
 * History page: two sub-views (recent measurements + full history).
 * Bottom tab navigation included.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_history.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_list_cont;

/* Navigation callback */

static void tab_btn_cb(lv_event_t *e)
{
  page_id_t page = (page_id_t)(uintptr_t)lv_event_get_user_data(e);
  ui_manager_switch_page(page);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/** Create a history list item card */

static void create_history_item(lv_obj_t *parent, const char *name,
                                  const char *result, const char *score_str,
                                  const char *time_ago)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, CONTENT_WIDTH, 56);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  /* Test name */

  lv_obj_t *name_lbl = lv_label_create(card);
  lv_label_set_text(name_lbl, name);
  lv_obj_set_style_text_color(name_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_16, 0);
  lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

  /* Result and score */

  lv_obj_t *result_lbl = lv_label_create(card);
  lv_label_set_text_fmt(result_lbl, "%s  Score %s", result, score_str);
  lv_obj_set_style_text_color(result_lbl, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(result_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(result_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  /* Time ago (right side) */

  if (time_ago && time_ago[0])
    {
      lv_obj_t *time_lbl = lv_label_create(card);
      lv_label_set_text(time_lbl, time_ago);
      lv_obj_set_style_text_color(time_lbl, COLOR_TEXT_DIM, 0);
      lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_14, 0);
      lv_obj_align(time_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
    }
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

      if (i == 1)
        {
          lv_obj_set_style_text_color(lbl, COLOR_GREEN, 0);
        }

      lv_obj_add_event_cb(btn, tab_btn_cb, LV_EVENT_CLICKED,
                           (void *)(uintptr_t)pages[i]);
    }

  return nav;
}

/** Populate the history list with sample/demo data */

static void populate_history(lv_obj_t *cont)
{
  app_context_t *ctx = app_get_context();

  /* Section title: Recent Measurements */

  lv_obj_t *sec_title = lv_label_create(cont);
  lv_label_set_text(sec_title, "Recent Measurements");
  lv_obj_set_style_text_color(sec_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sec_title, &lv_font_montserrat_14, 0);
  lv_obj_set_width(sec_title, CONTENT_WIDTH);

  /* Recent items (from context or demo data) */

  if (ctx->history_count > 0)
    {
      for (int i = 0; i < ctx->history_count && i < 5; i++)
        {
          fitness_record_t *rec = &ctx->history[i];
          char score_str[8];
          snprintf(score_str, sizeof(score_str), "%ld", (long)rec->score);
          char result_str[32];
          snprintf(result_str, sizeof(result_str), "%ld",
                   (long)rec->count);
          create_history_item(cont, rec->test_name, result_str,
                              score_str, "");
        }
    }
  else
    {
      /* Demo data */

      create_history_item(cont, "Jump Rope", "175 times", "90", "Just now");
      create_history_item(cont, "50m Run", "7.3 s", "85", "5 min ago");
      create_history_item(cont, "Long Jump", "2.28 m", "82", "12 min ago");
      create_history_item(cont, "Sit-ups", "48 times", "88", "");
      create_history_item(cont, "Pull-ups", "12 times", "78", "");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_history_create(lv_obj_t *parent)
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
  lv_label_set_text(ht, "History");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_20, 0);
  lv_obj_align(ht, LV_ALIGN_CENTER, 0, 0);

  /* Category label */

  lv_obj_t *cat = lv_label_create(page);
  lv_label_set_text(cat, "All Test Items");
  lv_obj_set_style_text_color(cat, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(cat, &lv_font_montserrat_14, 0);
  lv_obj_align(cat, LV_ALIGN_TOP_LEFT, SAFE_MARGIN, 50);

  /* Scrollable list */

  g_list_cont = lv_obj_create(page);
  lv_obj_set_size(g_list_cont, CONTENT_WIDTH, SCREEN_HEIGHT - 120);
  lv_obj_set_style_bg_opa(g_list_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_list_cont, 0, 0);
  lv_obj_set_style_pad_all(g_list_cont, 0, 0);
  lv_obj_set_style_pad_row(g_list_cont, 6, 0);
  lv_obj_set_flex_flow(g_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_list_cont, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align(g_list_cont, LV_ALIGN_TOP_MID, 0, 68);

  populate_history(g_list_cont);

  /* Bottom navigation */

  create_bottom_nav(page);

  return page;
}

void ui_history_update(void)
{
  /* Could refresh history data here */
}

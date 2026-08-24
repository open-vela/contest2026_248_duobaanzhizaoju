/****************************************************************************
 * apps/examples/fitness/ui_settings.c
 *
 * Settings page (设置): personal info, bluetooth toggle, wifi toggle, about.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_settings.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void tab_btn_cb(lv_event_t *e)
{
  page_id_t page = (page_id_t)(uintptr_t)lv_event_get_user_data(e);
  ui_manager_switch_page(page);
}

static void bt_toggle_cb(lv_event_t *e)
{
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
  app_context_t *ctx = app_get_context();
  ctx->settings.bluetooth_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
}

static void wifi_toggle_cb(lv_event_t *e)
{
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
  app_context_t *ctx = app_get_context();
  ctx->settings.wifi_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
}

static void test_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_TEST);
}

/** Create a settings section with a label */

static lv_obj_t *create_section(lv_obj_t *parent, const char *title)
{
  lv_obj_t *sec = lv_label_create(parent);
  lv_label_set_text(sec, title);
  lv_obj_set_style_text_color(sec, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sec, &lv_font_montserrat_14, 0);
  lv_obj_set_width(sec, CONTENT_WIDTH);
  return sec;
}

/** Create a settings row with label and optional right widget */

static lv_obj_t *create_setting_row(lv_obj_t *parent, const char *label,
                                      const char *right_text)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(row, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(row, 8, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 10, 0);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(row);
  lv_label_set_text(lbl, label);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

  if (right_text)
    {
      lv_obj_t *rt = lv_label_create(row);
      lv_label_set_text(rt, right_text);
      lv_obj_set_style_text_color(rt, COLOR_TEXT_DIM, 0);
      lv_obj_set_style_text_font(rt, &lv_font_montserrat_14, 0);
      lv_obj_align(rt, LV_ALIGN_RIGHT_MID, 0, 0);
    }

  return row;
}

/** Create a toggle row */

static lv_obj_t *create_toggle_row(lv_obj_t *parent, const char *label,
                                     bool initial_state,
                                     lv_event_cb_t cb)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(row, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(row, 8, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 10, 0);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(row);
  lv_label_set_text(lbl, label);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *sw = lv_switch_create(row);
  lv_obj_set_size(sw, 44, 24);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(sw, COLOR_PROGRESS_BG, 0);
  lv_obj_set_style_bg_color(sw, COLOR_GREEN, LV_PART_INDICATOR);
  if (initial_state)
    {
      lv_obj_add_state(sw, LV_STATE_CHECKED);
    }

  lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, NULL);

  return row;
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

      if (i == 3)
        {
          lv_obj_set_style_text_color(lbl, COLOR_GREEN, 0);
        }

      lv_obj_add_event_cb(btn, tab_btn_cb, LV_EVENT_CLICKED,
                           (void *)(uintptr_t)pages[i]);
    }

  return nav;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_settings_create(lv_obj_t *parent)
{
  app_context_t *ctx = app_get_context();

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
  lv_label_set_text(ht, "Settings");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_20, 0);
  lv_obj_align(ht, LV_ALIGN_CENTER, 0, 0);

  /* Scrollable content */

  lv_obj_t *content = lv_obj_create(page);
  lv_obj_set_size(content, CONTENT_WIDTH, SCREEN_HEIGHT - 120);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_set_style_pad_row(content, 6, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 52);

  /* Personal info card */

  lv_obj_t *info_card = lv_obj_create(content);
  lv_obj_set_size(info_card, CONTENT_WIDTH, 80);
  lv_obj_set_style_bg_color(info_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(info_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(info_card, 12, 0);
  lv_obj_set_style_border_width(info_card, 0, 0);
  lv_obj_set_style_pad_all(info_card, 12, 0);
  lv_obj_remove_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *name_lbl = lv_label_create(info_card);
  lv_label_set_text(name_lbl, STUDENT_NAME);
  lv_obj_set_style_text_color(name_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_20, 0);
  lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *id_lbl = lv_label_create(info_card);
  lv_label_set_text(id_lbl, "ID: " STUDENT_ID);
  lv_obj_set_style_text_color(id_lbl, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(id_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(id_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  lv_obj_t *class_lbl = lv_label_create(info_card);
  lv_label_set_text(class_lbl, "M  Grade 9 Class 2");
  lv_obj_set_style_text_color(class_lbl, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(class_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(class_lbl, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  /* Connection section */

  create_section(content, "Connection");
  create_toggle_row(content, "Bluetooth Pairing",
                     ctx->settings.bluetooth_on, bt_toggle_cb);
  create_toggle_row(content, "WiFi Sync",
                     ctx->settings.wifi_on, wifi_toggle_cb);

  /* About section */

  create_section(content, "About");
  create_setting_row(content, "Xsport", "v1.0.0");

  /* Device self-test button */

  lv_obj_t *test_btn = lv_button_create(content);
  lv_obj_set_size(test_btn, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(test_btn, COLOR_CARD_BG2, 0);
  lv_obj_set_style_radius(test_btn, 8, 0);
  lv_obj_set_style_border_width(test_btn, 0, 0);

  lv_obj_t *tb_lbl = lv_label_create(test_btn);
  lv_label_set_text(tb_lbl, "Device Self-Test");
  lv_obj_set_style_text_color(tb_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(tb_lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(tb_lbl);
  lv_obj_add_event_cb(test_btn, test_btn_cb, LV_EVENT_CLICKED, NULL);

  /* Bottom navigation */

  create_bottom_nav(page);

  return page;
}

void ui_settings_update(void)
{
  /* Could sync toggle states here */
}

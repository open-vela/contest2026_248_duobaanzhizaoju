/****************************************************************************
 * apps/examples/fitness/ui_test.c
 *
 * Device self-test page (设备自检): sensor status, firmware info.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_test.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void back_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_SETTINGS);
}

/** Create a sensor status row */

static void create_sensor_row(lv_obj_t *parent, const char *name,
                                bool ok)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, CONTENT_WIDTH - 16, 36);
  lv_obj_set_style_bg_color(row, COLOR_CARD_BG2, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(row, 8, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 8, 0);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *name_lbl = lv_label_create(row);
  lv_label_set_text(name_lbl, name);
  lv_obj_set_style_text_color(name_lbl, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *status = lv_label_create(row);
  if (ok)
    {
      lv_label_set_text(status, "Normal");
      lv_obj_set_style_text_color(status, COLOR_GREEN, 0);
    }
  else
    {
      lv_label_set_text(status, "Disconnected");
      lv_obj_set_style_text_color(status, COLOR_TEXT_DIM, 0);
    }
  lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
  lv_obj_align(status, LV_ALIGN_RIGHT_MID, 0, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_test_create(lv_obj_t *parent)
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
  lv_label_set_text(ht, "Device Self-Test");
  lv_obj_set_style_text_color(ht, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(ht, &lv_font_montserrat_16, 0);
  lv_obj_align(ht, LV_ALIGN_LEFT_MID, 44, 0);

  /* Device info card */

  lv_obj_t *dev_card = lv_obj_create(page);
  lv_obj_set_size(dev_card, CONTENT_WIDTH, 64);
  lv_obj_set_style_bg_color(dev_card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(dev_card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(dev_card, 12, 0);
  lv_obj_set_style_border_width(dev_card, 0, 0);
  lv_obj_set_style_pad_all(dev_card, 12, 0);
  lv_obj_remove_flag(dev_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(dev_card, LV_ALIGN_TOP_MID, 0, 56);

  lv_obj_t *fw = lv_label_create(dev_card);
  lv_label_set_text(fw, "SF32LB52 Firmware v1.0.0");
  lv_obj_set_style_text_color(fw, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(fw, &lv_font_montserrat_16, 0);
  lv_obj_align(fw, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *user = lv_label_create(dev_card);
  lv_label_set_text(user, "User: " STUDENT_NAME);
  lv_obj_set_style_text_color(user, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(user, &lv_font_montserrat_14, 0);
  lv_obj_align(user, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  /* Sensor Status section */

  lv_obj_t *sensor_title = lv_label_create(page);
  lv_label_set_text(sensor_title, "Sensor Status");
  lv_obj_set_style_text_color(sensor_title, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sensor_title, &lv_font_montserrat_14, 0);
  lv_obj_align(sensor_title, LV_ALIGN_TOP_LEFT, SAFE_MARGIN, 132);

  /* Sensor list */

  lv_obj_t *sensor_list = lv_obj_create(page);
  lv_obj_set_size(sensor_list, CONTENT_WIDTH, 180);
  lv_obj_set_style_bg_opa(sensor_list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(sensor_list, 0, 0);
  lv_obj_set_style_pad_all(sensor_list, 0, 0);
  lv_obj_set_style_pad_row(sensor_list, 4, 0);
  lv_obj_set_flex_flow(sensor_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(sensor_list, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(sensor_list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(sensor_list, LV_ALIGN_TOP_MID, 0, 152);

  create_sensor_row(sensor_list, "6-Axis IMU", true);
  create_sensor_row(sensor_list, "Heart Rate Sensor", true);
  create_sensor_row(sensor_list, "GPS Module", false);
  create_sensor_row(sensor_list, "Display", true);

  /* One-key self-test button */

  lv_obj_t *test_btn = lv_button_create(page);
  lv_obj_set_size(test_btn, CONTENT_WIDTH, 44);
  lv_obj_set_style_bg_color(test_btn, COLOR_GREEN, 0);
  lv_obj_set_style_radius(test_btn, 22, 0);
  lv_obj_set_style_border_width(test_btn, 0, 0);
  lv_obj_align(test_btn, LV_ALIGN_TOP_MID, 0, 348);

  lv_obj_t *tb = lv_label_create(test_btn);
  lv_label_set_text(tb, "Run Self-Test");
  lv_obj_set_style_text_color(tb, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(tb, &lv_font_montserrat_16, 0);
  lv_obj_center(tb);

  return page;
}

void ui_test_update(void)
{
  app_context_t *ctx = app_get_context();

  /* Set device status based on context */

  ctx->imu_ok = true;
  ctx->hr_sensor_ok = true;
  ctx->gps_ok = false;
  ctx->display_ok = true;
}

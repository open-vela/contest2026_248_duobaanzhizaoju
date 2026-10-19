/****************************************************************************
 * apps/examples/fitness/ui_select.c
 *
 * 体测项目选择页（对应设计稿 ui-screen-02-test-selection）：
 *   - 居中标题 + 青色下划线
 *   - 全宽列表行：项目图标 + 项目名 + 右侧灰色副标题
 *   - 点击行：启动对应项目算法并进入实时页
 *   - 底部返回箭头
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_select.h"
#include "ui_manager.h"
#include "ui_sport.h"
#include "data_collector.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ROW_HEIGHT   52
#define ROW_GAP      10
#define LIST_TOP     64
#define LIST_BOTTOM  (SCREEN_HEIGHT - 52)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void back_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_HOME);
}

static void row_cb(lv_event_t *e)
{
  int idx = (int)(uintptr_t)lv_event_get_user_data(e);
  const sport_profile_t *p = sport_profile_get(idx);

  if (p == NULL)
    {
      return;
    }

  sport_profile_select(idx);
  data_collector_start_sport(p->type);
  ui_sport_reset();
  ui_manager_switch_page(PAGE_SPORT);
}

/** 创建一行项目 */

static void create_sport_row(lv_obj_t *parent, int idx)
{
  const sport_profile_t *p = sport_profile_get(idx);

  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, CONTENT_WIDTH, ROW_HEIGHT);
  lv_obj_set_style_bg_color(row, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(row, 12, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

  /* 项目图标 */

  lv_obj_t *icon = lv_label_create(row);
  lv_label_set_text(icon, p->icon);
  lv_obj_set_style_text_color(icon, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(icon, FONT_SYM);
  lv_obj_align(icon, LV_ALIGN_LEFT_MID, 16, 0);

  /* 项目名 */

  lv_obj_t *name = lv_label_create(row);
  lv_label_set_text(name, p->name);
  lv_obj_set_style_text_color(name, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(name, FONT_CN);
  lv_obj_align(name, LV_ALIGN_LEFT_MID, 56, 0);

  /* 副标题 */

  lv_obj_t *hint = lv_label_create(row);
  lv_label_set_text(hint, p->hint);
  lv_obj_set_style_text_color(hint, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(hint, FONT_CN);
  lv_obj_align(hint, LV_ALIGN_RIGHT_MID, -16, 0);

  lv_obj_add_event_cb(row, row_cb, LV_EVENT_CLICKED,
                      (void *)(uintptr_t)idx);
}

/** 页面标题 + 青色下划线 */

static void create_title(lv_obj_t *parent, const char *text)
{
  lv_obj_t *t = lv_label_create(parent);
  lv_label_set_text(t, text);
  lv_obj_set_style_text_color(t, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(t, FONT_CN);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 16);

  lv_obj_t *line = lv_obj_create(parent);
  lv_obj_set_size(line, 56, 4);
  lv_obj_set_style_bg_color(line, COLOR_GREEN, 0);
  lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(line, 2, 0);
  lv_obj_set_style_border_width(line, 0, 0);
  lv_obj_set_style_pad_all(line, 0, 0);
  lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 44);
}

/** 底部返回箭头 */

static void create_back_btn(lv_obj_t *parent)
{
  lv_obj_t *back = lv_button_create(parent);
  lv_obj_set_size(back, 72, 36);
  lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(back, 0, 0);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -8);

  lv_obj_t *lbl = lv_label_create(back);
  lv_label_set_text(lbl, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(lbl, FONT_SYM);
  lv_obj_center(lbl);
  lv_obj_add_event_cb(back, back_btn_cb, LV_EVENT_CLICKED, NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_select_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  create_title(page, "选择体测项目");

  /* 可滚动列表 */

  lv_obj_t *list = lv_obj_create(page);
  lv_obj_set_size(list, CONTENT_WIDTH, LIST_BOTTOM - LIST_TOP);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, LIST_TOP);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 0, 0);
  lv_obj_set_style_pad_row(list, ROW_GAP, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);

  for (int i = 0; i < sport_profile_count(); i++)
    {
      create_sport_row(list, i);
    }

  create_back_btn(page);

  return page;
}

void ui_select_update(void)
{
  /* 列表为静态结构，无需刷新 */
}

/****************************************************************************
 * apps/examples/fitness/ui_splash.c
 *
 * Splash/loading page: Xsport branding with loading spinner.
 * Dark background, green accent "Xsport" title, loading animation.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_splash.h"
#include "ui_manager.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_spinner;
static void (*g_done_cb)(void);
static lv_timer_t *g_timer;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void splash_timer_cb(lv_timer_t *timer)
{
  /* After 2 seconds, switch to home page */

  ui_manager_switch_page(PAGE_HOME);
  lv_timer_delete(g_timer);
  g_timer = NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_splash_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  /* App title: "Xsport" in green */

  lv_obj_t *title = lv_label_create(page);
  lv_label_set_text(title, "Xsport");
  lv_obj_set_style_text_color(title, COLOR_GREEN, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -50);

  /* Subtitle: "中学生智能体测" */

  lv_obj_t *subtitle = lv_label_create(page);
  lv_label_set_text(subtitle, "Smart Fitness Test");
  lv_obj_set_style_text_color(subtitle, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, 0);
  lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -10);

  /* Version info */

  lv_obj_t *ver = lv_label_create(page);
  lv_label_set_text(ver, "SF32LB52  v1.0.0");
  lv_obj_set_style_text_color(ver, COLOR_TEXT_DIM, 0);
  lv_obj_set_style_text_font(ver, &lv_font_montserrat_14, 0);
  lv_obj_align(ver, LV_ALIGN_CENTER, 0, 15);

  /* Loading spinner */

  g_spinner = lv_spinner_create(page);
  lv_spinner_set_anim_params(g_spinner, 1000, 200);
  lv_obj_set_size(g_spinner, 40, 40);
  lv_obj_align(g_spinner, LV_ALIGN_CENTER, 0, 60);
  lv_obj_set_style_arc_color(g_spinner, COLOR_GREEN, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(g_spinner, COLOR_PROGRESS_BG, LV_PART_MAIN);
  lv_obj_set_style_arc_width(g_spinner, 3, 0);

  /* Auto-advance timer */

  g_timer = lv_timer_create(splash_timer_cb, 2000, NULL);
  lv_timer_set_repeat_count(g_timer, 1);

  return page;
}

void ui_splash_set_done_cb(void (*cb)(void))
{
  g_done_cb = cb;
}

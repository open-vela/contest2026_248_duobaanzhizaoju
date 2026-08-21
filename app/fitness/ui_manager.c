/****************************************************************************
 * apps/examples/fitness/ui_manager.c
 *
 * Page manager: handles page creation, switching, and lifecycle.
 * Bottom navigation: Home (运动), History (历史), Report (报告), Settings (设置)
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_manager.h"
#include "ui_splash.h"
#include "ui_home.h"
#include "ui_sport.h"
#include "ui_result.h"
#include "ui_detail.h"
#include "ui_history.h"
#include "ui_report.h"
#include "ui_settings.h"
#include "ui_test.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_pages[PAGE_COUNT];
static lv_obj_t *g_layer;
static page_id_t g_current = PAGE_SPLASH;

static lv_obj_t *g_tabview;
static lv_obj_t *g_tab_btns;

/* Bottom navigation tab names */

static const char *g_tab_names[] = {
  LV_SYMBOL_HOME " Sport",
  LV_SYMBOL_LIST " History",
  LV_SYMBOL_FILE " Report",
  LV_SYMBOL_SETTINGS " Settings",
  NULL
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void tab_changed_cb(lv_event_t *e)
{
  lv_obj_t *tv = (lv_obj_t *)lv_event_get_user_data(e);
  uint32_t tab_idx = lv_tabview_get_tab_active(tv);

  switch (tab_idx)
    {
      case 0:
        ui_manager_switch_page(PAGE_HOME);
        break;
      case 1:
        ui_manager_switch_page(PAGE_HISTORY);
        break;
      case 2:
        ui_manager_switch_page(PAGE_REPORT);
        break;
      case 3:
        ui_manager_switch_page(PAGE_SETTINGS);
        break;
      default:
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void ui_manager_init(void)
{
  lv_obj_t *scr = lv_screen_active();

  /* Set dark background */

  lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  /* Create the content layer */

  g_layer = lv_obj_create(scr);
  lv_obj_set_size(g_layer, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_center(g_layer);
  lv_obj_set_style_bg_opa(g_layer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_layer, 0, 0);
  lv_obj_set_style_pad_all(g_layer, 0, 0);
  lv_obj_remove_flag(g_layer, LV_OBJ_FLAG_SCROLLABLE);

  /* Create all pages */

  g_pages[PAGE_SPLASH] = ui_splash_create(g_layer);
  g_pages[PAGE_HOME] = ui_home_create(g_layer);
  g_pages[PAGE_SPORT] = ui_sport_create(g_layer);
  g_pages[PAGE_RESULT] = ui_result_create(g_layer);
  g_pages[PAGE_DETAIL] = ui_detail_create(g_layer);
  g_pages[PAGE_HISTORY] = ui_history_create(g_layer);
  g_pages[PAGE_REPORT] = ui_report_create(g_layer);
  g_pages[PAGE_SETTINGS] = ui_settings_create(g_layer);
  g_pages[PAGE_TEST] = ui_test_create(g_layer);

  /* Hide all except splash */

  for (int i = 0; i < PAGE_COUNT; i++)
    {
      if (i != PAGE_SPLASH)
        {
          lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

  g_current = PAGE_SPLASH;
}

void ui_manager_switch_page(page_id_t page)
{
  if (page >= PAGE_COUNT || page == g_current)
    {
      return;
    }

  /* Hide current page */

  lv_obj_add_flag(g_pages[g_current], LV_OBJ_FLAG_HIDDEN);

  /* Show target page */

  lv_obj_remove_flag(g_pages[page], LV_OBJ_FLAG_HIDDEN);

  g_current = page;

  /* Refresh data when entering certain pages */

  switch (page)
    {
      case PAGE_HOME:
        ui_home_update();
        break;
      case PAGE_SPORT:
        ui_sport_update();
        break;
      case PAGE_RESULT:
        ui_result_update();
        break;
      case PAGE_DETAIL:
        ui_detail_update();
        break;
      case PAGE_HISTORY:
        ui_history_update();
        break;
      case PAGE_REPORT:
        ui_report_update();
        break;
      case PAGE_SETTINGS:
        ui_settings_update();
        break;
      case PAGE_TEST:
        ui_test_update();
        break;
      default:
        break;
    }
}

lv_obj_t *ui_manager_get_page(page_id_t page)
{
  if (page < PAGE_COUNT)
    {
      return g_pages[page];
    }

  return NULL;
}

void ui_manager_refresh(void)
{
  switch (g_current)
    {
      case PAGE_HOME:
        ui_home_update();
        break;
      case PAGE_SPORT:
        ui_sport_update();
        break;
      default:
        break;
    }
}

void ui_manager_update_sport(void)
{
  if (g_current == PAGE_SPORT)
    {
      ui_sport_update();
    }
}

void ui_manager_update_home(void)
{
  if (g_current == PAGE_HOME)
    {
      ui_home_update();
    }
}

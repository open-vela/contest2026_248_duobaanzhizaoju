/****************************************************************************
 * apps/examples/fitness/ui_badges.c
 *
 * 勋章成就页（对应设计稿 ui-screen-08-badges）：
 *   - 等级卡：奖杯 + "运动达人 Lv.5" + 升级进度条（每 600 积分一级）
 *   - 统计卡：已获勋章 / 总运动次数 / 总运动时长
 *   - 我的勋章：3×3 网格，未解锁显示灰色 "？"
 *   - 底部返回箭头
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ui_badges.h"
#include "ui_manager.h"
#include "data_collector.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BADGE_COUNT      9
#define BADGE_COLS       3
#define CELL_W           (CONTENT_WIDTH / BADGE_COLS)
#define CELL_H           58
#define BADGE_DIA        40
#define GRID_TOP         222
#define POINTS_PER_LEVEL 600

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  const char *icon;       /* 勋章符号（解锁后显示） */
  const char *name;       /* 勋章名 */
  uint32_t    color_hex;  /* 底色 */
  bool        (*check)(app_context_t *ctx);   /* 解锁条件 */
} badge_def_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_level_name;
static lv_obj_t *g_level_sub;
static lv_obj_t *g_level_fill;
static lv_obj_t *g_stat_badges;
static lv_obj_t *g_stat_sessions;
static lv_obj_t *g_stat_hours;
static lv_obj_t *g_badge_circle[BADGE_COUNT];
static lv_obj_t *g_badge_icon[BADGE_COUNT];
static lv_obj_t *g_badge_name[BADGE_COUNT];

static const badge_def_t g_badge_defs[BADGE_COUNT] =
{
  { LV_SYMBOL_LOOP,    "跳绳达人", 0xFFD54A, NULL },
  { LV_SYMBOL_PLAY,    "跑步新手", 0x2DD4BF, NULL },
  { LV_SYMBOL_CHART,   "心率大师", 0xFFD54A, NULL },
  { LV_SYMBOL_REFRESH, "一周坚持", 0x2DD4BF, NULL },
  { LV_SYMBOL_OK,      "满分成就", 0xFFD54A, NULL },
  { LV_SYMBOL_CHARGE,  "燃脂先锋", 0x2DD4BF, NULL },
  { NULL,              "???",      0x2C2C2E, NULL },
  { NULL,              "???",      0x2C2C2E, NULL },
  { NULL,              "???",      0x2C2C2E, NULL }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void back_btn_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ui_manager_switch_page(PAGE_HOME);
}

/** 解锁条件（引用 g_badge_defs 中无法直接初始化为函数） */

static bool check_jump_rope(app_context_t *c)
{
  for (int i = 0; i < c->history_count; i++)
    {
      if (strcmp(c->history[i].test_type, TEST_TYPE_JUMP_ROPE) == 0)
        {
          return true;
        }
    }

  return false;
}

static bool check_any_run(app_context_t *c)
{
  for (int i = 0; i < c->history_count; i++)
    {
      const sport_profile_t *p = sport_profile_find(c->history[i].test_type);

      if (p != NULL && p->metric == SPORT_METRIC_TIME)
        {
          return true;
        }
    }

  return false;
}

static bool check_max_hr(app_context_t *c)
{
  for (int i = 0; i < c->history_count; i++)
    {
      if (c->history[i].max_hr >= 170)
        {
          return true;
        }
    }

  return false;
}

static bool check_sessions(app_context_t *c)
{
  return c->total_sessions >= 7;
}

static bool check_full_score(app_context_t *c)
{
  for (int i = 0; i < c->history_count; i++)
    {
      if (c->history[i].score >= 100)
        {
          return true;
        }
    }

  return false;
}

static bool check_fat_burn(app_context_t *c)
{
  for (int i = 0; i < c->history_count; i++)
    {
      if (c->history[i].avg_hr >= 120)
        {
          return true;
        }
    }

  return false;
}

/** 等级称号 */

static const char *level_name(int32_t level)
{
  if (level < 3)
    {
      return "运动新手";
    }

  if (level < 5)
    {
      return "运动健将";
    }

  return "运动达人";
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

/** 勋章网格单元 */

static void create_badge_cell(lv_obj_t *parent, int idx)
{
  int col = idx % BADGE_COLS;
  int row = idx / BADGE_COLS;
  int cx = col * CELL_W + CELL_W / 2;

  lv_obj_t *circle = lv_obj_create(parent);
  lv_obj_set_size(circle, BADGE_DIA, BADGE_DIA);
  lv_obj_set_style_radius(circle, BADGE_DIA / 2, 0);
  lv_obj_set_style_bg_color(circle, COLOR_CARD_BG2, 0);
  lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(circle, 0, 0);
  lv_obj_set_style_pad_all(circle, 0, 0);
  lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(circle, cx - BADGE_DIA / 2, row * CELL_H);

  lv_obj_t *ic = lv_label_create(circle);
  lv_label_set_text(ic, "?");
  lv_obj_set_style_text_color(ic, COLOR_TEXT_DIM, 0);
  lv_obj_set_style_text_font(ic, FONT_SYM);
  lv_obj_center(ic);

  lv_obj_t *nm = lv_label_create(parent);
  lv_label_set_text(nm, "???");
  lv_obj_set_style_text_color(nm, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(nm, FONT_CN);
  lv_obj_set_width(nm, CELL_W - 8);
  lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(nm, cx - (CELL_W - 8) / 2,
                 row * CELL_H + BADGE_DIA + 2);

  g_badge_circle[idx] = circle;
  g_badge_icon[idx] = ic;
  g_badge_name[idx] = nm;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *ui_badges_create(lv_obj_t *parent)
{
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(page, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  create_title(page, "勋章成就");

  /* 等级卡 */

  lv_obj_t *card = lv_obj_create(page);
  lv_obj_set_size(card, CONTENT_WIDTH, 70);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 58);

  lv_obj_t *trophy = lv_obj_create(card);
  lv_obj_set_size(trophy, 44, 44);
  lv_obj_set_style_radius(trophy, 22, 0);
  lv_obj_set_style_bg_color(trophy, COLOR_YELLOW, 0);
  lv_obj_set_style_bg_opa(trophy, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(trophy, 0, 0);
  lv_obj_set_style_pad_all(trophy, 0, 0);
  lv_obj_remove_flag(trophy, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(trophy, LV_ALIGN_LEFT_MID, 12, 0);

  lv_obj_t *t_ic = lv_label_create(trophy);
  lv_label_set_text(t_ic, LV_SYMBOL_CHART);
  lv_obj_set_style_text_color(t_ic, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(t_ic, FONT_SYM);
  lv_obj_center(t_ic);

  g_level_name = lv_label_create(card);
  lv_label_set_text(g_level_name, "运动达人 Lv.1");
  lv_obj_set_style_text_color(g_level_name, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_level_name, FONT_CN);
  lv_obj_align(g_level_name, LV_ALIGN_TOP_LEFT, 68, 8);

  g_level_sub = lv_label_create(card);
  lv_label_set_text(g_level_sub, "距离下一级还需 -- 积分");
  lv_obj_set_style_text_color(g_level_sub, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(g_level_sub, FONT_CN);
  lv_obj_align(g_level_sub, LV_ALIGN_TOP_LEFT, 68, 30);

  lv_obj_t *prog_bg = lv_obj_create(card);
  lv_obj_set_size(prog_bg, CONTENT_WIDTH - 80, 4);
  lv_obj_set_style_bg_color(prog_bg, COLOR_PROGRESS_BG, 0);
  lv_obj_set_style_bg_opa(prog_bg, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(prog_bg, 2, 0);
  lv_obj_set_style_border_width(prog_bg, 0, 0);
  lv_obj_set_style_pad_all(prog_bg, 0, 0);
  lv_obj_remove_flag(prog_bg, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(prog_bg, LV_ALIGN_BOTTOM_LEFT, 68, -8);

  g_level_fill = lv_obj_create(prog_bg);
  lv_obj_set_size(g_level_fill, 0, 4);
  lv_obj_set_style_bg_color(g_level_fill, COLOR_GREEN, 0);
  lv_obj_set_style_bg_opa(g_level_fill, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(g_level_fill, 2, 0);
  lv_obj_set_style_border_width(g_level_fill, 0, 0);
  lv_obj_set_style_pad_all(g_level_fill, 0, 0);
  lv_obj_remove_flag(g_level_fill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(g_level_fill, LV_ALIGN_LEFT_MID, 0, 0);

  /* 统计卡：已获勋章 / 总运动次数 / 总运动时长 */

  lv_obj_t *stats = lv_obj_create(page);
  lv_obj_set_size(stats, CONTENT_WIDTH, 58);
  lv_obj_set_style_bg_color(stats, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(stats, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(stats, 14, 0);
  lv_obj_set_style_border_width(stats, 0, 0);
  lv_obj_set_style_pad_all(stats, 0, 0);
  lv_obj_remove_flag(stats, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(stats, LV_ALIGN_TOP_MID, 0, 136);

  g_stat_badges = lv_label_create(stats);
  lv_label_set_text(g_stat_badges, "0");
  lv_obj_set_style_text_color(g_stat_badges, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_stat_badges, FONT_NUM_S);
  lv_obj_align(g_stat_badges, LV_ALIGN_TOP_MID, -120, 6);

  lv_obj_t *sb_u = lv_label_create(stats);
  lv_label_set_text(sb_u, "已获勋章");
  lv_obj_set_style_text_color(sb_u, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sb_u, FONT_CN);
  lv_obj_align(sb_u, LV_ALIGN_BOTTOM_MID, -120, -6);

  g_stat_sessions = lv_label_create(stats);
  lv_label_set_text(g_stat_sessions, "0");
  lv_obj_set_style_text_color(g_stat_sessions, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_stat_sessions, FONT_NUM_S);
  lv_obj_align(g_stat_sessions, LV_ALIGN_TOP_MID, 0, 6);

  lv_obj_t *ss_u = lv_label_create(stats);
  lv_label_set_text(ss_u, "总运动次数");
  lv_obj_set_style_text_color(ss_u, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(ss_u, FONT_CN);
  lv_obj_align(ss_u, LV_ALIGN_BOTTOM_MID, 0, -6);

  g_stat_hours = lv_label_create(stats);
  lv_label_set_text(g_stat_hours, "0");
  lv_obj_set_style_text_color(g_stat_hours, COLOR_TEXT_PRIMARY, 0);
  lv_obj_set_style_text_font(g_stat_hours, FONT_NUM_S);
  lv_obj_align(g_stat_hours, LV_ALIGN_TOP_MID, 120, 6);

  lv_obj_t *sh_u = lv_label_create(stats);
  lv_label_set_text(sh_u, "总时长/小时");
  lv_obj_set_style_text_color(sh_u, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(sh_u, FONT_CN);
  lv_obj_align(sh_u, LV_ALIGN_BOTTOM_MID, 120, -6);

  /* 我的勋章 */

  lv_obj_t *mine = lv_label_create(page);
  lv_label_set_text(mine, "我的勋章");
  lv_obj_set_style_text_color(mine, COLOR_TEXT_SECONDARY, 0);
  lv_obj_set_style_text_font(mine, FONT_CN);
  lv_obj_align(mine, LV_ALIGN_TOP_LEFT, SAFE_MARGIN, 202);

  lv_obj_t *grid = lv_obj_create(page);
  lv_obj_set_size(grid, CONTENT_WIDTH, SCREEN_HEIGHT - GRID_TOP - 48);
  lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, GRID_TOP);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < BADGE_COUNT; i++)
    {
      create_badge_cell(grid, i);
    }

  create_back_btn(page);

  return page;
}

void ui_badges_update(void)
{
  app_context_t *ctx = app_get_context();
  char buf[48];
  int earned = 0;
  int into_level;

  /* 等级卡 */

  snprintf(buf, sizeof(buf), "%s Lv.%ld", level_name(ctx->level),
           (long)ctx->level);
  lv_label_set_text(g_level_name, buf);

  into_level = ctx->points - (ctx->level - 1) * POINTS_PER_LEVEL;

  snprintf(buf, sizeof(buf), "距离 Lv.%ld 还需 %ld 积分",
           (long)(ctx->level + 1),
           (long)(POINTS_PER_LEVEL - into_level));
  lv_label_set_text(g_level_sub, buf);

  lv_obj_set_size(g_level_fill,
                  (CONTENT_WIDTH - 80) * into_level / POINTS_PER_LEVEL, 4);

  /* 统计卡（勋章数在下方解锁判定后回填） */

  snprintf(buf, sizeof(buf), "%ld", (long)ctx->total_sessions);
  lv_label_set_text(g_stat_sessions, buf);

  snprintf(buf, sizeof(buf), "%.1f", (double)ctx->total_sec / 3600.0);
  lv_label_set_text(g_stat_hours, buf);

  /* 勋章解锁状态 */

  for (int i = 0; i < BADGE_COUNT; i++)
    {
      bool unlocked = false;

      if (i < 6)
        {
          switch (i)
            {
              case 0:
                unlocked = check_jump_rope(ctx);
                break;
              case 1:
                unlocked = check_any_run(ctx);
                break;
              case 2:
                unlocked = check_max_hr(ctx);
                break;
              case 3:
                unlocked = check_sessions(ctx);
                break;
              case 4:
                unlocked = check_full_score(ctx);
                break;
              case 5:
                unlocked = check_fat_burn(ctx);
                break;
              default:
                break;
            }
        }

      if (unlocked)
        {
          earned++;
          lv_obj_set_style_bg_color(g_badge_circle[i],
                                    lv_color_hex(g_badge_defs[i].color_hex),
                                    0);
          lv_label_set_text(g_badge_icon[i], g_badge_defs[i].icon);
          lv_obj_set_style_text_color(g_badge_icon[i],
                                      lv_color_hex(0x000000), 0);
          lv_label_set_text(g_badge_name[i], g_badge_defs[i].name);
          lv_obj_set_style_text_color(g_badge_name[i], COLOR_TEXT_PRIMARY, 0);
        }
      else
        {
          lv_obj_set_style_bg_color(g_badge_circle[i], COLOR_CARD_BG2, 0);
          lv_label_set_text(g_badge_icon[i], "?");
          lv_obj_set_style_text_color(g_badge_icon[i], COLOR_TEXT_DIM, 0);
          lv_label_set_text(g_badge_name[i], "???");
          lv_obj_set_style_text_color(g_badge_name[i],
                                      COLOR_TEXT_SECONDARY, 0);
        }
    }

  snprintf(buf, sizeof(buf), "%ld", (long)earned);
  lv_label_set_text(g_stat_badges, buf);
}

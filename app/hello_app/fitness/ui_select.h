/****************************************************************************
 * apps/examples/fitness/ui_select.h
 *
 * 体测项目选择页：网格展示全部体测项目，点击进入实时测试。
 ****************************************************************************/

#ifndef __UI_SELECT_H
#define __UI_SELECT_H

#include "ui_common.h"

lv_obj_t *ui_select_create(lv_obj_t *parent);
void ui_select_update(void);

#endif /* __UI_SELECT_H */

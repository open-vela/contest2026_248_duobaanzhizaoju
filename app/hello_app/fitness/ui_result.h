/****************************************************************************
 * apps/examples/fitness/ui_result.h
 *
 * Score result page: overall score, metrics, detail/home buttons.
 ****************************************************************************/

#ifndef __UI_RESULT_H
#define __UI_RESULT_H

#include "ui_common.h"

lv_obj_t *ui_result_create(lv_obj_t *parent);
void ui_result_update(void);

#endif /* __UI_RESULT_H */

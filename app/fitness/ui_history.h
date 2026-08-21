/****************************************************************************
 * apps/examples/fitness/ui_history.h
 *
 * History page: recent measurements and full history list.
 ****************************************************************************/

#ifndef __UI_HISTORY_H
#define __UI_HISTORY_H

#include "ui_common.h"

lv_obj_t *ui_history_create(lv_obj_t *parent);
void ui_history_update(void);

#endif /* __UI_HISTORY_H */

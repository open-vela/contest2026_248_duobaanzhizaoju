/****************************************************************************
 * apps/examples/fitness/ui_sport.h
 *
 * Real-time sport tracking page: count, timer, heart rate, start/stop.
 ****************************************************************************/

#ifndef __UI_SPORT_H
#define __UI_SPORT_H

#include "ui_common.h"

lv_obj_t *ui_sport_create(lv_obj_t *parent);
void ui_sport_update(void);
void ui_sport_reset(void);

#endif /* __UI_SPORT_H */

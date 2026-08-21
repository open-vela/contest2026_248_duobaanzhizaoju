/****************************************************************************
 * apps/examples/fitness/ui_home.h
 *
 * Home page: greeting, metric cards, heart rate waveform, bottom nav.
 ****************************************************************************/

#ifndef __UI_HOME_H
#define __UI_HOME_H

#include "ui_common.h"

lv_obj_t *ui_home_create(lv_obj_t *parent);
void ui_home_update(void);

#endif /* __UI_HOME_H */

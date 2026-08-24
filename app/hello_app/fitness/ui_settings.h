/****************************************************************************
 * apps/examples/fitness/ui_settings.h
 *
 * Settings page: personal info, bluetooth, wifi, about.
 ****************************************************************************/

#ifndef __UI_SETTINGS_H
#define __UI_SETTINGS_H

#include "ui_common.h"

lv_obj_t *ui_settings_create(lv_obj_t *parent);
void ui_settings_update(void);

#endif /* __UI_SETTINGS_H */

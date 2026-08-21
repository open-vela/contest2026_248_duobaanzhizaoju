/****************************************************************************
 * apps/examples/fitness/ui_splash.h
 *
 * Splash/loading page: Xsport branding and loading animation.
 ****************************************************************************/

#ifndef __UI_SPLASH_H
#define __UI_SPLASH_H

#include "ui_common.h"

lv_obj_t *ui_splash_create(lv_obj_t *parent);
void ui_splash_set_done_cb(void (*cb)(void));

#endif /* __UI_SPLASH_H */

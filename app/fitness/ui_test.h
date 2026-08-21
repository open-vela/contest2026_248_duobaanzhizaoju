/****************************************************************************
 * apps/examples/fitness/ui_test.h
 *
 * Device self-test page: sensor status, firmware info.
 ****************************************************************************/

#ifndef __UI_TEST_H
#define __UI_TEST_H

#include "ui_common.h"

lv_obj_t *ui_test_create(lv_obj_t *parent);
void ui_test_update(void);

#endif /* __UI_TEST_H */

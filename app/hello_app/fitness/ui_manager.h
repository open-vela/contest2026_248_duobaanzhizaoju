/****************************************************************************
 * apps/examples/fitness/ui_manager.h
 *
 * Page manager: handles page creation, switching, and lifecycle.
 ****************************************************************************/

#ifndef __UI_MANAGER_H
#define __UI_MANAGER_H

#include "ui_common.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void ui_manager_init(void);
void ui_manager_switch_page(page_id_t page);
lv_obj_t *ui_manager_get_page(page_id_t page);
void ui_manager_refresh(void);

/* Update specific pages */

void ui_manager_update_sport(void);
void ui_manager_update_home(void);

#endif /* __UI_MANAGER_H */

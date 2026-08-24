/****************************************************************************
 * apps/examples/fitness/data_collector.h
 *
 * Data collector: sensor data collection, fitness test logic,
 * and data upload via HTTP.
 ****************************************************************************/

#ifndef __DATA_COLLECTOR_H
#define __DATA_COLLECTOR_H

#include "ui_common.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void data_collector_init(void);
void data_collector_stop(void);

/* Sport session control */

void data_collector_start_sport(const char *test_type);
void data_collector_stop_sport(void);
void data_collector_pause_sport(void);
void data_collector_resume_sport(void);

/* Get current data */

const sport_data_t *data_collector_get_sport_data(void);

/* Calculate score for a test result */

int32_t calculate_score(const char *test_type, int32_t count, float time_sec);

#endif /* __DATA_COLLECTOR_H */

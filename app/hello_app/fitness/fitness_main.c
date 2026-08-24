/****************************************************************************
 * apps/examples/fitness/fitness_main.c
 *
 * Main entry point for the Middle School Fitness Test LVGL application.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include "ui_manager.h"
#include "data_collector.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

  if (lv_is_initialized())
    {
      return -1;
    }

#ifdef NEED_BOARDINIT
  boardctl(BOARDIOC_INIT, 0);
#endif

  /* Initialize LVGL */

  lv_init();

  /* Configure NuttX display driver */

  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_FITNESS_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      return 1;
    }

#ifdef CONFIG_INPUT_TOUCHSCREEN
  /* Wait for touchscreen device */

  {
    int retry;
    for (retry = 0; retry < 20; retry++)
      {
        int fd = open(CONFIG_EXAMPLES_FITNESS_INPUT_DEVPATH, O_RDONLY);
        if (fd >= 0)
          {
            close(fd);
            break;
          }

        usleep(200000);
      }
  }
#endif

  /* Initialize the UI */

  ui_manager_init();

  /* Initialize the data collector */

  data_collector_init();

  /* Main event loop */

  while (1)
    {
      uint32_t idle;
      idle = lv_timer_handler();
      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }

  /* Cleanup (unreachable) */

  data_collector_stop();
  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}

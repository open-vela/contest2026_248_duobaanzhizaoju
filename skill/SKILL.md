---
name: openvela-lvgl-app
description: "Use when building LVGL apps for OpenVela/NuttX devices."
triggers:
  - "LVGL application on OpenVela or NuttX"
  - "Smartwatch UI with circular display"
  - "Embedded GUI development for wearable devices"
  - "NuttX application with LVGL graphics"
  - "SF32LB52 board touch screen not working"
  - "FT6146 touch driver not compiling"
  - "Kconfig CONFIG_INPUT_FT6146 missing from .config"
  - "defconfig settings dropped during cmake configure"
---

# OpenVela LVGL Application Development

Build graphical applications for OpenVela (NuttX-based) using LVGL v9, targeting circular smartwatch displays.

## Step 0: Explore Existing Examples

Before writing any code, examine existing LVGL examples in the project to learn conventions:

```
apps/examples/lvgldemo/     # Reference LVGL demo
apps/examples/lvglterm/     # Terminal-style LVGL
apps/graphics/lvgl/lvgl/    # LVGL library source
```

Read the build files (Makefile, Make.defs, CMakeLists.txt, Kconfig) and the main .c file from an existing example. These define the exact patterns to follow.

## Step 1: Create Build System Files

Follow the NuttX app structure exactly. Four files are required:

### Kconfig
- Use `menuconfig EXAMPLES_<APPNAME>` (tristate)
- Depend on `GRAPHICS_LVGL`
- Define: priority, stack size, touchscreen device path, and app-specific configs
- Stack size: 32768 minimum for simple apps, 65536 for apps with networking

### Makefile
```makefile
include $(APPDIR)/Make.defs
PROGNAME  = <appname>
PRIORITY  = $(CONFIG_EXAMPLES_<APPNAME>_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_<APPNAME>_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_<APPNAME>)
CSRCS     = file1.c file2.c ...
include $(APPDIR)/Application.mk
```

### Make.defs
```makefile
ifneq ($(CONFIG_EXAMPLES_<APPNAME>),)
CONFIGURED_APPS += $(APPDIR)/examples/<appname>
endif
```

### CMakeLists.txt
```cmake
if(CONFIG_EXAMPLES_<APPNAME>)
  nuttx_add_application(NAME <appname> PRIORITY ... STACKSIZE ... MODULE ...)
    DEPENDS lvgl SRCS file1.c file2.c)
endif()
```

## Step 2: Main Entry Point Pattern

```c
#include <nuttx/config.h>
#include <unistd.h>
#include <sys/boardctl.h>
#include <lvgl/lvgl.h>

#undef NEED_BOARDINIT
#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

  if (lv_is_initialized()) { return -1; }
#ifdef NEED_BOARDINIT
  boardctl(BOARDIOC_INIT, 0);
#endif
  lv_init();
  lv_nuttx_dsc_init(&info);
#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#elif defined(CONFIG_VIDEO_FB)
  info.fb_path = "/dev/fb0";
#endif
#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_<APPNAME>_INPUT_DEVPATH;
#endif
  lv_nuttx_init(&info, &result);
  if (result.disp == NULL) { return 1; }

  /* Create UI here */

  while (1) {
    uint32_t idle = lv_timer_handler();
    idle = idle ? idle : 1;
    usleep(idle * 1000);
  }

  lv_nuttx_deinit(&result);
  lv_deinit();
  return 0;
}
```

For libuv event loop, wrap with `lv_nuttx_uv_init()` / `uv_run()`.

## Step 3: LVGL v9 API Key Differences from v8

| v8 | v9 |
|----|-----|
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_btn_create()` | `lv_button_create()` |
| `lv_indev_get_act()` | `lv_indev_active()` |
| `lv_obj_set_style_local_*()` | `lv_obj_set_style_*()` |

v8 compatibility macros exist in `lv_api_map_v8.h` but prefer native v9 APIs.

### LVGL v9 Object Creation
```c
lv_obj_t *obj = lv_obj_create(parent);      // Container
lv_obj_t *btn = lv_button_create(parent);   // Button
lv_obj_t *lbl = lv_label_create(parent);    // Label
lv_obj_t *bar = lv_bar_create(parent);      // Progress bar
lv_obj_t *sld = lv_slider_create(parent);   // Slider
lv_obj_t *sw  = lv_switch_create(parent);   // Toggle switch
```

### Style System (v9)
```c
lv_obj_set_style_bg_color(obj, color, 0);          // Default part
lv_obj_set_style_bg_color(obj, color, LV_STATE_PRESSED); // State
lv_obj_set_style_bg_color(obj, color, LV_PART_INDICATOR); // Part
lv_obj_set_style_radius(obj, 12, 0);
lv_obj_set_style_border_width(obj, 0, 0);
lv_obj_set_style_pad_all(obj, 8, 0);
lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
```

### Flex Layout (v9)
```c
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                       LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
```

### Events
```c
lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, user_data);
```

## Step 4: Circular Display UI Patterns

For round smartwatch screens (e.g., 454×454):

- **Safe margin = 50px** for 454px circle (inscribed square: 454/√2 ≈ 321px, margin = (454-321)/2 ≈ 66px, use 50px as practical compromise)
- `#define SAFE_MARGIN 50` / `#define CONTENT_WIDTH (SCREEN_WIDTH - 2 * SAFE_MARGIN)`
- All UI elements must stay within `CONTENT_WIDTH` range
- Use `lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, margin)` for top-aligned content
- Center critical elements with `lv_obj_center()`
- Use high-contrast dark theme (black bg, white text) for OLED
- Large touch targets (120px+ for primary buttons on 454px)
- Avoid text near circular edges
- LVGL does NOT clip to circle automatically — design within safe margins

## Step 5: Networking on NuttX

For HTTP communication, use POSIX sockets directly:
- `gethostbyname()` for DNS
- Standard `socket()` / `connect()` / `send()` / `recv()`
- Set `SO_RCVTIMEO` and `SO_SNDTIMEO` for timeouts
- For HTTPS: requires mbedTLS or wolfSSL configuration
- Connection: `close()` after each request (HTTP/1.1 with `Connection: close`)

## Pitfalls

1. **LVGL v8 vs v9 API**: Don't mix APIs. Use v9 native functions. The v8 compat macros work but v9 style is preferred.
2. **Stack size**: Pure UI apps need 32768. Apps with sensor data collection need 32768-65536. Apps with networking/threads need 65536+. **Never use pthreads for LVGL** — threads calling LVGL functions cause stack overflow crashes.
3. **Circular clipping**: LVGL doesn't clip to circle automatically. Design UI within safe margins (50px for 454px display).
4. **Thread safety**: LVGL is NOT thread-safe. Do NOT create pthreads that call LVGL functions. Use `lv_timer_create()` for periodic updates, or call data updates from the main loop before `lv_timer_handler()`.
5. **NuttX config macros**: All config values use `CONFIG_` prefix. Access via `CONFIG_EXAMPLES_<APPNAME>_<OPTION>`.
6. **Font availability**: Enable needed fonts in defconfig (`CONFIG_LV_FONT_MONTSERRAT_14=y` etc). Not all sizes are enabled by default.
7. **`lv_button_create` vs `lv_btn_create`**: In v9, use `lv_button_create()`. `lv_btn_create` is a v8 compat macro.
8. **BES2800 display**: Uses `CONFIG_VIDEO_FB=y` → `/dev/fb0`, NOT `/dev/lcd0`. Do NOT enable `CONFIG_LV_USE_NUTTX_LCD`. Set `info.fb_path = "/dev/fb0"` in code. **Always use `#elif defined(CONFIG_VIDEO_FB)` fallback in main.**
9. **BES2800 linker**: BSP lib `libbeschip_ap.a` conflicts with NuttX `arm_nputs.c`. Add `add_link_options(-Wl,--allow-multiple-definition)` to `vendor/bes/boards/.../cmake/Toolchain.cmake`.
10. **BES2800 build**: Use `./1700_ap.sh` script, NOT raw `build.sh`. Output: `cmake_out/best1700_ep/aos_evb/out/nuttx_ap.bin`. **Always re-run `1700_ap.sh` after defconfig changes** — `ninja` alone won't pick up new configs.
11. **BES2800 autostart**: Edit `vendor/bes/.../src/etc/init.d/rcS.ap` to add `myapp &` under `#ifdef CONFIG_EXAMPLES_MYAPP`. The rcS.last `#include` may not work reliably.
12. **Avoid pthreads for LVGL**: LVGL is not thread-safe. Background threads calling LVGL functions cause stack overflow crashes. Instead, call `data_update()` from the main loop before `lv_timer_handler()`.
13. **BES2800 ccache**: If `ccache: not found` during build, create a passthrough stub: `printf '#!/bin/bash\nexec "${@}"' > /usr/local/bin/ccache && chmod +x /usr/local/bin/ccache`
14. **defconfig → .config sync**: Modifying defconfig does NOT auto-update `.config`. Must re-run `1700_ap.sh` (which hashes defconfig and cleans build dir if changed). Running `ninja` after `lunch` uses stale config.
15. **wapi command syntax**: ALL wapi parameters are NUMERIC indices, not strings. `wapi essid wlan0 "SSID" 1` (not "on"). `wapi psk wlan0 "pass" 3` (not "CCMP"). SSID is set via `essid`, NOT as a `psk` argument. See `references/wapi-command-syntax.md`.
16. **BMI270 on BES1700**: Uses `/dev/imu0` character device (NOT sensor framework `/dev/sensor_accel0`). Data is `struct accel_gyro_st_s` with int16_t raw values. Wiring: I2C1 SCL=G36, SDA=G37, INT1=G40. See `references/wifi-and-sensor-integration.md`.
17. **LV_SYMBOL_SLEEP**: Does NOT exist in LVGL v9. Use `LV_SYMBOL_PAUSE` instead.
18. **FFmpeg dependency**: `CONFIG_MEDIA=y` requires FFmpeg headers not in source tree. Disable `CONFIG_MEDIA=y`, `CONFIG_MEDIA_SERVER=y`, `CONFIG_MEDIA_TOOL=y` if not needed.
19. **mk2cmake.py**: BES SDK build tool missing from source tree. Create `/opt/openvela_2026/prebuild/mk2cmake.py` to convert `board_cfg.mk` to `board_cfg.cmake`.
20. **FT6146 touch driver not compiling (SF32LB52)**: If `CONFIG_INPUT_FT6146=y` is in defconfig but missing from `.config` after cmake configure, the root cause is usually a missing `CONFIG_I2C=y`. The FT6146 Kconfig has `depends on INPUT && I2C && INPUT_TOUCHSCREEN`. The SiFli BSP uses `CONFIG_BSP_USING_I2C=y` which is a different symbol — the NuttX-level `CONFIG_I2C=y` is also required. Check with `grep "I2C=y" .config`.
21. **Board Kconfig empty → driver Kconfig not sourced**: If the board's `Kconfig` file is empty (comment-only), peripheral driver configs like `INPUT_FT6146` are invisible to the Kconfig system. The fix: add `source "/absolute/path/to/board/../drivers/Kconfig"` to the board Kconfig. Use absolute paths — `$BINDIR` and `rsource` resolve relative to the cmake build directory, not the source tree.
22. **Verifying defconfig settings survive cmake configure**: After cmake configure, always check `.config` for critical settings: `grep "YOUR_CONFIG" cmake_out/<board>/.config`. Settings that don't appear were dropped by `olddefconfig` due to unsatisfied dependencies or missing Kconfig symbols. Fix the dependency chain, not the `.config` file (manual edits to `.config` are overwritten on next configure).
23. **drivers/Kconfig $BINDIR path**: The board's `drivers/Kconfig` uses `$BINDIR` which resolves to `cmake_out/<board>/` (the build directory), not the source tree. If Kconfig files are in the source tree, change `$BINDIR/...` to absolute paths like `/opt/openvela_2026/vendor/...`.

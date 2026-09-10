# Lessons from Middle School Fitness Test App (390×450 Rectangular)

Multi-page fitness tracking app with 11 screens, bottom tab navigation, real-time heart rate waveform, background sensor simulation, and HTTP data upload.

## Build Workflow (SF32LB52-DevKit-LCD)

```bash
cd /opt/openvela_2026

# 1. cmake configure
cmake -B cmake_out/sf32lb52_fitness -S "$PWD/nuttx" -GNinja \
  -DBOARD_CONFIG=../vendor/sifli/boards/sf32lb52/sf32lb52_devkit_lcd/configs/fitness \
  -DEXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations"

# 2. compile
cmake --build cmake_out/sf32lb52_fitness

# 3. flash
sftool -c SF32LB52 -p /dev/ttyUSB0 -b 1000000 \
  --before default_reset --after soft_reset \
  write_flash cmake_out/sf32lb52_fitness/nuttx.bin@0x12010000

# 4. run
picocom -b 1000000 --noreset --lower-rts --lower-dtr /dev/ttyUSB0
nsh> fitness
```

## defconfig Checklist for SF32LB52 with Touch

```
# App
CONFIG_EXAMPLES_FITNESS=y
CONFIG_EXAMPLES_FITNESS_SCREEN_WIDTH=390
CONFIG_EXAMPLES_FITNESS_SCREEN_HEIGHT=450
CONFIG_EXAMPLES_FITNESS_STACKSIZE=65536
CONFIG_EXAMPLES_FITNESS_SERVER_URL="http://101.35.231.154:9000"

# LVGL fonts (ALL sizes used in source must be listed)
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_20=y
CONFIG_LV_FONT_MONTSERRAT_24=y
CONFIG_LV_FONT_MONTSERRAT_28=y
CONFIG_LV_FONT_MONTSERRAT_36=y

# Display
CONFIG_LCD=y
CONFIG_LCD_DEV=y
CONFIG_LCD_FRAMEBUFFER=y
CONFIG_LCD_USING_CO5300=y
CONFIG_LV_USE_NUTTX_LCD=y

# Touch screen (ALL THREE required — see pitfalls)
CONFIG_I2C=y
CONFIG_INPUT_FT6146=y
CONFIG_TOUCH_IRQ_PIN=41
CONFIG_INPUT_TOUCHSCREEN=y
CONFIG_LV_USE_NUTTX_TOUCHSCREEN=y
```

## Touch Screen Debugging (FT6146 on SF32LB52)

### Symptom: Display works, touch has no response

### Root cause chain

```
CONFIG_INPUT_FT6146=y missing from .config
  ← Kconfig: depends on INPUT && I2C && INPUT_TOUCHSCREEN
    ← CONFIG_I2C=y NOT set (only BSP_USING_I2C exists)
      ← board Kconfig empty, drivers/Kconfig not sourced
```

### Step-by-step diagnosis

1. Check `.config` for the driver:
   ```bash
   grep "FT6146\|TOUCH_IRQ" cmake_out/<board>/.config
   # Empty = driver not compiled
   ```

2. Check dependencies in `.config`:
   ```bash
   grep -E "^CONFIG_INPUT=y|^CONFIG_I2C=y|^CONFIG_INPUT_TOUCHSCREEN=y" .config
   # All three must be present
   ```

3. Check if Kconfig symbol exists (board Kconfig must source drivers):
   ```bash
   cat vendor/.../boards/<board>/Kconfig
   # If empty → driver Kconfig symbols are invisible
   ```

4. Verify ft6146.c is in build.ninja:
   ```bash
   grep "ft6146" cmake_out/<board>/build.ninja
   # Empty = source file not compiled
   ```

### Three-part fix

**Fix 1**: Board Kconfig must source drivers:
```kconfig
# vendor/sifli/boards/sf32lb52/sf32lb52_devkit_lcd/Kconfig
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/Kconfig"
```

**Fix 2**: drivers/Kconfig must use absolute paths (not `$BINDIR`):
```kconfig
# vendor/sifli/boards/sf32lb52/drivers/Kconfig
menu "SiFli On-Board Device Drivers"
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/platform/lcd/Kconfig"
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/platform/input/Kconfig"
endmenu
```

**Fix 3**: Add `CONFIG_I2C=y` to defconfig (the missing dependency):
```
# SiFli BSP uses BSP_USING_I2C, but FT6146 Kconfig needs the NuttX-level I2C
CONFIG_I2C=y
```

### Verification after fix

```bash
# Reconfigure (clean rebuild)
rm -rf cmake_out/<board>
cmake -B cmake_out/<board> -S "$PWD/nuttx" -GNinja -DBOARD_CONFIG=...

# Check .config
grep "FT6146\|TOUCH_IRQ\|I2C=y" cmake_out/<board>/.config
# Should show: CONFIG_INPUT_FT6146=y, CONFIG_TOUCH_IRQ_PIN=41, CONFIG_I2C=y

# Check build.ninja
grep "ft6146" cmake_out/<board>/build.ninja
# Should show ft6146.c.o build rules

# Compile and verify binary size increased (~100KB for touch driver)
```

## Multi-Page Architecture Pattern

For apps with many pages (5+), use a page manager instead of lv_tabview:

```c
typedef enum {
  PAGE_SPLASH = 0,
  PAGE_HOME,
  PAGE_SPORT,
  PAGE_COUNT
} page_id_t;

static lv_obj_t *g_pages[PAGE_COUNT];

// Create all pages, hide all except first
for (int i = 0; i < PAGE_COUNT; i++) {
  g_pages[i] = <page>_create(g_layer);
  if (i != PAGE_SPLASH)
    lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
}

// Switch by hide/show
void ui_manager_switch_page(page_id_t page) {
  lv_obj_add_flag(g_pages[g_current], LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(g_pages[page], LV_OBJ_FLAG_HIDDEN);
  g_current = page;
}
```

## Bottom Tab Navigation Pattern

LVGL v9 on NuttX does NOT have `lv_tabview`. Build bottom nav manually:

```c
static lv_obj_t *create_bottom_nav(lv_obj_t *parent) {
  lv_obj_t *nav = lv_obj_create(parent);
  lv_obj_set_size(nav, SCREEN_WIDTH, 50);
  lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(nav, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(nav, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(nav, 0, 0);
  lv_obj_set_style_pad_all(nav, 0, 0);

  const char *icons[] = { LV_SYMBOL_HOME, LV_SYMBOL_LIST, ... };
  const char *labels[] = { "Sport", "History", "Report", "Settings" };
  page_id_t pages[] = { PAGE_HOME, PAGE_HISTORY, PAGE_REPORT, PAGE_SETTINGS };
  int btn_w = SCREEN_WIDTH / 4;

  for (int i = 0; i < 4; i++) {
    lv_obj_t *btn = lv_button_create(nav);
    lv_obj_set_size(btn, btn_w, 46);
    lv_obj_set_pos(btn, i * btn_w, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    // label with icon + text
    // active tab gets green text, others get secondary color
    lv_obj_add_event_cb(btn, tab_btn_cb, LV_EVENT_CLICKED,
                         (void *)(uintptr_t)pages[i]);
  }
  return nav;
}
```

Key: Each page with bottom nav creates its own copy. Active tab gets green text.

## Custom Waveform Drawing (LVGL v9)

```c
// Register draw callback
lv_obj_add_event_cb(waveform_obj, waveform_draw_cb, LV_EVENT_DRAW_MAIN, NULL);

static void waveform_draw_cb(lv_event_t *e) {
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t coords;
  lv_obj_get_coords(lv_event_get_target(e), &coords);

  // Build points array
  lv_point_precise_t points[60];  // v9 uses lv_point_precise_t, NOT lv_point_t
  for (int i = 0; i < 60; i++) {
    points[i].x = (lv_value_precise_t)(coords.x1 + i * w / 59);
    points[i].y = (lv_value_precise_t)(coords.y2 - data[i] * h / range);
  }

  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = COLOR_GREEN;
  dsc.width = 2;
  dsc.p1 = points[0];       // Direct assignment, NOT pointer
  for (int i = 1; i < 60; i++) {
    dsc.p2 = points[i];     // Direct assignment
    lv_draw_line(layer, &dsc);
    dsc.p1 = points[i];
  }
}
```

**CRITICAL**: In LVGL v9, `lv_draw_line_dsc_t.p1/p2` are `lv_point_precise_t` values, NOT `lv_point_t*` pointers. Using `&points[i]` causes compile error.

## Rectangular Display Layout (390×450)

- Safe margin: 12px
- Cards: full CONTENT_WIDTH with 12px radius
- 3-card row: `CONTENT_WIDTH / 3 - 6` per card
- 2-card row: `CONTENT_WIDTH / 2 - 6` per card
- Bottom nav: 50px height, 4 equal buttons

## Dark Theme Color Palette

```c
#define COLOR_BG            lv_color_hex(0x111111)
#define COLOR_CARD_BG       lv_color_hex(0x1E1E2E)
#define COLOR_TEXT_PRIMARY   lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_SECONDARY lv_color_hex(0x999999)
#define COLOR_GREEN          lv_color_hex(0x00E676)
#define COLOR_RED            lv_color_hex(0xFF3B30)
```

## Background Data Collector Pattern

Use pthread for data collection, update UI from main LVGL thread:

```c
static void *collector_thread(void *arg) {
  while (g_running) {
    simulate_sport_data(&g_app_ctx.sport);
    ui_manager_update_sport();  // Update LVGL labels/bars
    usleep(1000000);
  }
  return NULL;
}
```

LVGL is NOT thread-safe. On single-core Cortex-M33, calling LVGL functions from a single background thread works if the main loop isn't doing concurrent operations.

## Pitfalls

1. **No lv_tabview in NuttX LVGL v9**: Must build bottom nav manually.
2. **lv_point_precise_t in v9 line drawing**: `p1`/`p2` are values, not pointers. Using `&points[i]` causes `incompatible types` error.
3. **Multiple nav copies**: Each page with bottom nav creates its own nav bar.
4. **Waveform redraw**: Call `lv_obj_invalidate()` after updating data buffer.
5. **CONFIG_I2C=y vs BSP_USING_I2C**: These are different symbols. FT6146 Kconfig needs `CONFIG_I2C=y`.
6. **Board Kconfig empty**: Driver configs become invisible to Kconfig system.
7. **$BINDIR in Kconfig**: Resolves to cmake build dir, not source tree. Use absolute paths.
8. **defconfig → .config**: Always verify critical settings survive cmake configure with `grep`.
9. **Manual .config edits**: Lost on next cmake configure. Fix the Kconfig chain instead.

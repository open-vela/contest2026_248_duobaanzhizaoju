# Lessons Learned from Elderly Health Care App Session

## Font Configuration (Critical)

### Problem
Only `lv_font_montserrat_14` is enabled by default. Using other sizes causes `undeclared` compile errors.

### Solution
Add ALL used font sizes to defconfig before building:
```
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_20=y
CONFIG_LV_FONT_MONTSERRAT_24=y
CONFIG_LV_FONT_MONTSERRAT_28=y
CONFIG_LV_FONT_MONTSERRAT_36=y
```

### Pre-build Checklist
```bash
# Find all font references in source
grep -rn "lv_font_montserrat" apps/examples/<appname>/*.c | grep -oP 'lv_font_montserrat_\d+' | sort -u
# Verify each is enabled
grep "LV_FONT_MONTSERRAT" cmake_out/<board>/.config
```

## CJK Font Handling

### Available CJK Font
`lv_font_simsun_16_cjk` - SimSun 16px with common CJK glyphs + FontAwesome icons.

### Enable
```
CONFIG_LV_FONT_SIMSUN_16_CJK=y
```

### Size Impact
Adds ~200KB to binary. Consider English text for resource-constrained devices.

### Usage Pattern
```c
#define FONT_CHINESE  (&lv_font_simsun_16_cjk)
lv_obj_set_style_text_font(label, FONT_CHINESE, 0);
```

## Network Stack Configuration

### Problem 1: Socket functions undefined
`send`, `recv`, `connect`, `gethostbyname`, `socket`, `setsockopt` not linked.

### Solution: Conditional Compilation
```c
#ifdef CONFIG_NET
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

int http_post(...) {
#ifdef CONFIG_NET
  /* real implementation */
#else
  (void)path; (void)token;  // NOT LV_UNUSED() - that's LVGL-only
  return -ENOSYS;
#endif
}
```

### Problem 2: `arm_netinitialize` undefined
Caused by `CONFIG_NET=y` without actual network hardware.

### Solution
Do NOT add `CONFIG_NET=y` unless board has WiFi/Ethernet/RNDIS. Keep `#ifdef CONFIG_NET` guards.

### Problem 3: `LV_UNUSED` undefined in non-LVGL files
`LV_UNUSED()` is an LVGL macro from `<lvgl/lvgl.h>`.

### Solution
Use `(void)var;` in standalone C files (http_client.c, data_collector.c, etc.).

## Build System

### Never Edit .config Directly
Manual `.config` edits cause stale objects → `memcpy` undefined errors.

### Correct Workflow
```bash
# 1. Edit defconfig
vim vendor/<vendor>/boards/<chip>/<board>/configs/<app>/defconfig

# 2. Clean rebuild
rm -rf cmake_out/<board>
cmake -B cmake_out/<board> -S "$PWD/nuttx" -GNinja \
  -DBOARD_CONFIG=../vendor/<vendor>/boards/<chip>/<board>/configs/<app> \
  -DEXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations"
cmake --build cmake_out/<board>
```

## Display Resolution Discovery

### Check Actual Hardware (Don't Trust Mockups)
```bash
# Find LCD resolution from driver Kconfig
grep "LCD_HOR_RES_MAX\|LCD_VER_RES_MAX" vendor/<vendor>/boards/<chip>/drivers/lcd/Kconfig
```

### SF32LB52 Boards
All three variants use the same CO5300 AMOLED: **390×450 rectangular** (not circular).

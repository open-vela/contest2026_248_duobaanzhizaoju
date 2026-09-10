# BES2800 Elderly App — Full Session Log

## Session: 2026-08-20, BES2800BP round watch, elderly health care app

### What was built
LVGL v9 elderly health monitoring app for BES2800 (best1700_ep) with 454×454 round AMOLED display. Multi-page UI: index (SOS + vitals + steps), health detail (HR history), settings, SOS alarm.

### Architecture decisions that worked
1. **Stub data_collector**: Replaced real sensor/network code with stub that provides static data. This avoided pthread + LVGL thread safety crashes.
2. **Removed http_client**: Not needed for initial bring-up on hardware without confirmed network.
3. **32KB stack**: Enough for UI-only app. 64KB+ caused memory conflicts with LVGL's 2MB pool.
4. **Safe margin 50px**: For 454px circular display, 50px keeps all content within the visible circle.
5. **rcS.ap modification**: Put autostart in rcS.ap (the existing UI app section), not rcS.last (which isn't reliably included).

### Critical config entries for BES2800 + LVGL
```
CONFIG_EXAMPLES_ELDERLY_BES=y
CONFIG_EXAMPLES_ELDERLY_BES_STACKSIZE=32768
CONFIG_LV_USE_NUTTX_LCD=y          # MUST HAVE - without it, NULL pointer crash
CONFIG_LV_FONT_MONTSERRAT_14=y     # Not enabled by default
CONFIG_LV_FONT_MONTSERRAT_16=y     # Not enabled by default
```

### Crash debugging sequence
1. **Stack Overflow (64KB)** → increased to 128KB → still overflow → 256KB → different crash
2. **NULL pointer at 0x90** → `CONFIG_LV_USE_NUTTX_LCD` was missing
3. **Stack Overflow again at 256KB** → pthread + LVGL conflict. Simplified to stub, reduced to 32KB
4. **Final working config**: 32KB stack, no pthreads, stub data_collector, LCD config enabled

### Circular display widget sizes (454×454)
- SOS button: 120px diameter (not 140px)
- Vital cards: 95×65px each (not 115×70px)
- Safe margin: 50px (not 12px or 30px)
- Status bar: 200×28px, centered
- Health status: 220×32px at bottom

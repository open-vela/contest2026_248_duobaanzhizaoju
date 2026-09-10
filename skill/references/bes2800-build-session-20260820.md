# BES2800 (best1700_ep) Build Pitfalls — Session 2026-08-20

## Official Build Script

**Always use `1700_ap.sh`** for BES1700 builds, NOT manual cmake/lunch:
```bash
cd /opt/openvela_2026
VELA_EXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations -Wno-format-overflow -Wno-format-truncation" bash 1700_ap.sh
```

The script:
- Hashes defconfig + `rc.sysinit.ap` to detect changes, forces clean rebuild on change
- Passes BES-specific AP_FLAGS (DSP, DMA, BT, web server configs)
- Copies artifacts to `cmake_out/best1700_ep/aos_evb/ap/` and `out/`
- Build dir: `cmake_out/aos_evb_ap` (NOT `cmake_out/bes1700_ap`)

**Never use** `lunch` + `m` for production builds — it bypasses the AP_FLAGS and artifact copying.

## Linker Fix: Toolchain.cmake, NOT build.ninja

**Symptom**: `multiple definition of 'up_nputs'`

**Fix**: Add to `vendor/bes/boards/best1700_ep/aos_evb/cmake/Toolchain.cmake` after `add_link_options(-Wl,--entry=Boot_Loader)`:
```cmake
  add_link_options(-Wl,--allow-multiple-definition)
```

**CRITICAL**: Do NOT modify `build.ninja` — `lunch` regenerates it, overwriting your fix. The Toolchain.cmake change persists across rebuilds.

## CONFIG_LV_USE_NUTTX_LCD Required

**Symptom**: `arm_usagefault: Data access violation` at `MMFAR: 0x00000090` during first `lv_timer_handler()`.

**Root cause**: `CONFIG_LV_USE_NUTTX_LCD` is NOT enabled by default. Without it, LVGL's NuttX driver doesn't create the display framebuffer.

**Fix**: Add to defconfig:
```
CONFIG_LV_USE_NUTTX_LCD=y
```

## Stack Size for Complex LVGL Apps

**Symptom**: `arm_usagefault: Stack Overflow` even with 128KB.

**Fix**: Set to **262144** (256KB) for apps with LVGL UI + background pthreads:
```
CONFIG_EXAMPLES_<APPNAME>_STACKSIZE=262144
```

Simple LVGL-only apps: 65536 (64KB) is sufficient.

## ccache Dependency

**Symptom**: `/bin/sh: 1: ccache: not found`

**Fix**: Create passthrough stub:
```bash
printf '#!/bin/bash\nexec "${@}"\n' > /usr/local/bin/ccache
chmod +x /usr/local/bin/ccache
```

## mk2cmake.py Missing

**Symptom**: `mk2cmake failed: python3: can't open file '.../prebuild/mk2cmake.py'`

**Fix**: Create `/opt/openvela_2026/prebuild/mk2cmake.py` that converts `board_cfg.mk` `export VAR := value` lines to cmake `set()` commands.

## Media Framework FFmpeg

**Symptom**: `fatal error: libavutil/mem.h: No such file or directory`

**Fix**: Remove `CONFIG_MEDIA=y`, `CONFIG_MEDIA_SERVER=y`, `CONFIG_MEDIA_TOOL=y` from defconfig.

## Font Config for BES1700

BES1700 defconfig has Montserrat 20-48. If code uses 14 or 16:
```
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
```

# Build Pitfalls Learned from Real Sessions

## Font Configuration Errors

**Symptom**: `'lv_font_montserrat_20' undeclared` (or 12, 16, 18, 22, 24, 28, 36)

**Root cause**: Only `lv_font_montserrat_14` is enabled by default in LVGL on NuttX.

**Fix**: Add to defconfig:
```
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_20=y
CONFIG_LV_FONT_MONTSERRAT_24=y
CONFIG_LV_FONT_MONTSERRAT_28=y
CONFIG_LV_FONT_MONTSERRAT_36=y
```

**Lesson**: Always grep your source for `lv_font_montserrat_` references, then verify each size is enabled in defconfig before building.

## Network Linker Errors

**Symptom**: `undefined reference to 'send'`, `'recv'`, `'connect'`, `'gethostbyname'`, `'socket'`, `'setsockopt'`

**Root cause**: Socket API not available because `CONFIG_NET` is not set.

**Fix**: Wrap all socket code in `#ifdef CONFIG_NET` guards. Return `-ENOSYS` in the else branch.

**Follow-on symptom**: `undefined reference to 'arm_netinitialize'`

**Root cause**: Enabling `CONFIG_NET=y` without network hardware. The arch layer expects a board-specific network init function.

**Fix**: Do NOT add `CONFIG_NET=y` unless the board has WiFi/Ethernet/RNDIS. Keep the `#ifdef CONFIG_NET` guards.

## Direct .config Editing

**Symptom**: `undefined reference to 'memcpy'` in `bchlib_read.c` / `bchlib_write.c`

**Root cause**: Directly editing `.config` in the build directory causes stale object files and inconsistent state.

**Fix**: Never edit `.config` directly. Always:
1. Modify the defconfig file
2. Delete the build directory: `rm -rf cmake_out/<board>`
3. Reconfigure: `cmake -B cmake_out/<board> -S "$PWD/nuttx" -GNinja -DBOARD_CONFIG=...`
4. Rebuild

## LV_UNUSED in Non-LVGL Files

**Symptom**: `undefined reference to 'LV_UNUSED'` at link time (compiles as implicit function declaration warning)

**Root cause**: `LV_UNUSED()` is an LVGL macro defined in LVGL headers. In standalone C files that don't include `<lvgl/lvgl.h>`, it's not available.

**Fix**: Use `(void)var;` instead of `LV_UNUSED(var);` in non-LVGL source files.

## SF32LB52 Serial Console

**Symptom**: SoC stays in reset, no output, or keeps rebooting

**Root cause**: SF32LB52 has no dedicated reset pin. RTS connects to a VCC load switch. Standard serial tools assert RTS on open.

**Fix**: Always use `picocom -b 1000000 --noreset --lower-rts --lower-dtr /dev/ttyUSB0`

**Never use**: `screen`, `cu`, or `minicom` (without careful RTS handling)

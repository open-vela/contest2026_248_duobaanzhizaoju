# BES2800 (best1700_ep) Build Pitfalls

## mk2cmake.py Missing

**Symptom**: `CMake Error: mk2cmake failed: python3: can't open file '.../prebuild/mk2cmake.py'`

**Root cause**: BES SDK expects a `prebuild/` directory at the openvela root containing `mk2cmake.py`. This converts `board_cfg.mk` to cmake format. It's not always included in the source tree.

**Fix**: Create `/opt/openvela_2026/prebuild/mk2cmake.py` — a simple parser that reads `export VAR := value` lines and `ifeq ($(CONFIG_XXX),y)` conditionals from .mk files and outputs cmake `set()` / `if()` equivalents.

## Linker: up_nputs Multiple Definition

**Symptom**: `multiple definition of 'up_nputs'; libbeschip_ap.a(bes_serial.c.o) vs arch/libarch.a(arm_nputs.c.o)`

**Root cause**: BES prebuilt BSP library (`libbeschip_ap.a`) defines `up_nputs` which conflicts with NuttX's `arch/arm/src/common/arm_nputs.c`.

**Fix**: Add `--allow-multiple-definition` to the linker flags. In `build.ninja`, replace `-Wl,--start-group` with `-Wl,--allow-multiple-definition -Wl,--start-group` in all link commands.

## Media Framework FFmpeg Dependency

**Symptom**: `fatal error: libavutil/mem.h: No such file or directory` in `media_plugin.c`

**Root cause**: `CONFIG_MEDIA=y` enables the media framework which depends on FFmpeg/libav headers not present in the source tree.

**Fix**: Remove `CONFIG_MEDIA=y`, `CONFIG_MEDIA_SERVER=y`, `CONFIG_MEDIA_TOOL=y` from defconfig if the app doesn't need multimedia.

## Format-Truncation Warnings (-Werror)

**Symptom**: `error: '%ld' directive output may be truncated [-Werror=format-truncation]` with `char buf[8]` and `%ld` format.

**Fix**: Increase buffer sizes from 8 to 16 for any buffer used with `%ld` snprintf. Or add `CFLAGS="-Wno-format-truncation"` to the build command.

## Format-Overflow Warnings (-Werror)

**Symptom**: `error: '%s' directive argument is null [-Werror=format-overflow]` in framework code (e.g., media_stub.c).

**Fix**: Add `-Wno-format-overflow` via CFLAGS:
```bash
CFLAGS="-Wno-format-overflow -Wno-format-truncation" m
```

## Build Command for BES1700

```bash
cd /opt/openvela_2026
source build/envsetup.sh
lunch vendor/bes/boards/best1700_ep/aos_evb/configs/ap cmake_out/bes1700_ap
CFLAGS="-Wno-format-overflow -Wno-format-truncation" m
```

Build uses `-j1` (single-threaded) by default. Takes ~15-25 minutes for a full build.

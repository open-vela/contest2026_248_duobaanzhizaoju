# BES2800 (best1700_ep) Board - Updated 2026-08-24

## Build
```bash
cd /opt/openvela_2026
VELA_EXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations -Wno-format-overflow -Wno-format-truncation" bash 1700_ap.sh
```

## Display: /dev/fb0 (NOT /dev/lcd0)
BES1700 uses `CONFIG_VIDEO_FB=y` → `/dev/fb0`.
Do NOT use `CONFIG_LV_USE_NUTTX_LCD=y`.

## Required defconfig additions
```
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
# CONFIG_MEDIA is not set
```

## Build fixes needed
1. Create `/opt/openvela_2026/prebuild/mk2cmake.py` (converter from board_cfg.mk)
2. Add `--allow-multiple-definition` to `Toolchain.cmake`
3. Remove CONFIG_MEDIA (FFmpeg missing)
4. Install ccache or create stub

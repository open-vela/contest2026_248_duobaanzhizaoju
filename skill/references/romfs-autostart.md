# NuttX ROMFS Autostart Mechanism

## How the Init Script Works

NuttX boots and runs the init script from ROMFS embedded in the firmware binary.

### Build chain
1. Board provides `src/etc/init.d/rcS` (C-preprocessor source)
2. `add_board_rcsrcs()` in CMakeLists.txt registers it for preprocessing
3. `nuttx_generate_preprocess_target()` runs the C preprocessor on rcS
4. Preprocessed output goes to `cmake_out/<board>/romfs_etc/init.d/rcS`
5. ROMFS image is generated and embedded in `nuttx.bin`

### rcS include chain
```
rcS
├── #include "rcS.first"     (early init)
├── #ifdef CONFIG_BES1700_AP
│   └── #include "rcS.ap"    (AP-specific init, may be commented out)
├── #include "rcS.last"      (late init - UNRELIABLE, see pitfall)
```

## Adding App Autostart

Edit `rcS.ap` and add before the existing UI app launch:

```c
#ifdef CONFIG_EXAMPLES_YOURAPP
yourapp &
#elif defined(CONFIG_BESWATCH)
beswatch &
#else
lvgldemo widgets &
#endif
```

The `&` (background) is **critical** — without it, NSH shell never starts.

## Verifying Autostart

After building:
```bash
# Check the preprocessed init script
cat cmake_out/<board>/romfs_etc/init.d/rcS

# Verify in the binary
strings nuttx.bin | grep "yourapp &"
```

## Pitfall: rcS.last Not Included

**Symptom**: Changes to `rcS.last` don't appear in the preprocessed output.

**Root cause**: The C preprocessor's include path may not cover the directory where `rcS.last` lives. The `add_board_rcsrcs()` only registers `rcS` and `rc.sysinit` — `rcS.last` is included via `#include` from within `rcS`, but the include path depends on cmake's `NUTTX_INCLUDE_DIRECTORIES`.

**Fix**: Always put autostart commands in `rcS.ap` (or `rcS.first`), which are known to be on the include path. Don't rely on `rcS.last`.

## Serial Console

### BES1700 (USB CDC ACM)
```bash
# Linux
picocom -b 921600 /dev/ttyACM0

# Windows
# COM port at 921600 baud, 8N1, no flow control
```

### SF32LB52 (UART)
```bash
picocom -b 1000000 --noreset --lower-rts --lower-dtr /dev/ttyUSB0
```

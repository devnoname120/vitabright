#include "hooks.h"
#include "../main.h"
#include "../log.h"
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

// Required in order to jump to code that is in thumb mode
#define THUMB_BIT 1

// Default value corresponding do a dimmed screen after inactivity
#define LCD_DIMMED_VALUE 25

static SceUID lcd_table_inject = -1;
static SceUID lcd_set_brightness_hook = -1;

static tai_hook_ref_t lcd_set_brightness_ref = -1;

static SceUID power_set_max_brightness_hook = -1;
static tai_hook_ref_t power_set_max_brightness_ref = 0;

// static uint8_t original_brightness_values[17] = {31,  37,  43,  50,  58,  67,
//                                             77,  88,  100, 114, 129, 147,
//                                             166, 182, 203, 227, 255};

static uint8_t lcd_brightness_values[17] = {
    1, 3, 5, 8, 13, 20, 29, 41, 57, 76, 95, 116, 137, 161, 190, 220, 255};

int (*ksceLcdGetBrightness)();
int (*ksceLcdSetBrightness)(unsigned int brightness);

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

int lcd_brightness_to_index(unsigned int brightness) {
  // 17 levels (0 to 16), brightness starts at 2, max 65536
  return 16 * (brightness - 2) / 65534;
}

int hook_ksceLcdSetBrightness(unsigned int brightness) {
  // Not trying to dim screen after inactivity
  if (brightness != 1) {
    return TAI_CONTINUE(int, lcd_set_brightness_ref, brightness);
  }

  int old_brightness = ksceLcdGetBrightness();
  int old_index = lcd_brightness_to_index(old_brightness);

  int new_brightness = old_brightness;
  // If this doesn't make the screen brighter
  if (old_brightness >= 2 && lcd_brightness_values[old_index] >= LCD_DIMMED_VALUE) {
    new_brightness = brightness;
  }

  return TAI_CONTINUE(int, lcd_set_brightness_ref, new_brightness);
}

int hook_kscePowerSetDisplayMaxBrightnessForLcd(int limit) {
  // Workaround for https://github.com/yifanlu/taiHEN/issues/12
  if (power_set_max_brightness_ref == 0) {
    return 0;
  }

  // Limits are not necessary on LCD.
  return TAI_CONTINUE(int, power_set_max_brightness_ref, 0x10000);
}

void lcd_enable_hooks() {
  // Completely remove max brightness limit when PS Vita is running in power mode C/D.
  power_set_max_brightness_hook = taiHookFunctionExportForKernel(KERNEL_PID, &power_set_max_brightness_ref, "ScePower", TAI_ANY_LIBRARY, 0x77027B6B, hook_kscePowerSetDisplayMaxBrightnessForLcd);
  LOG("[LCD] hooking kscePowerSetDisplayMaxBrightness: 0x%08X\n", power_set_max_brightness_hook);

  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  int ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceLcd", &info);
  LOG("[LCD] getmodninfo(\"SceLcd\"): 0x%08X\n", ret);
  LOG("[LCD] SceLcd modid: 0x%08X\n", info.modid);

  if (ret < 0) {
    LOG("[LCD] Couldn't get SceLcd module id! Abandoning...\n");
    return;
  }

  uint32_t lcd_table_off = 0;
  size_t ksceLcdGetBrightness_addr = 0;
  size_t ksceLcdSetBrightness_addr = 0;

  switch (sw_version >> 16) {
  case 0x360: {
    lcd_table_off = 0x1B00;
    ksceLcdGetBrightness_addr = 0x0EFC | THUMB_BIT;
    ksceLcdSetBrightness_addr = 0x1170 | THUMB_BIT;
    break;
  }

  case 0x365:
  case 0x367:
  case 0x368:
  case 0x369:
  case 0x370: {
    lcd_table_off = 0x1B48;
    ksceLcdGetBrightness_addr = 0x0F08 | THUMB_BIT;
    ksceLcdSetBrightness_addr = 0x117C | THUMB_BIT;
    break;
  }

  default: // Not supported
    LOG("[LCD] Unsupported OS version: 0x%08X. Abandoning...\n", sw_version);
    return;
  }

  LOG("[LCD] OS version: 0x%08X\n, table offset: 0x%08X, ksceLcdGetBrightness_addr: 0x%08X, "
      "ksceLcdSetBrightness_addr: 0x%08X\n",
      sw_version,
      (unsigned int)lcd_table_off,
      ksceLcdGetBrightness_addr,
      ksceLcdSetBrightness_addr);

  lcd_table_inject = taiInjectDataForKernel(KERNEL_PID,
                                            info.modid,
                                            0,
                                            lcd_table_off, // 0x1BA0?
                                            lcd_brightness_values,
                                            sizeof(lcd_brightness_values));
  LOG("[LCD] injectdata: 0x%08X\n", lcd_table_inject);

  int res_offset1 = module_get_offset(
      KERNEL_PID, info.modid, 0, ksceLcdGetBrightness_addr, (uintptr_t *)&ksceLcdGetBrightness);
  if (res_offset1 < 0) {
    LOG("[LCD] module_get_offset(\"ksceLcdGetBrightness\"): 0x%08X\n", res_offset1);
  }

  int res_offset2 = module_get_offset(
      KERNEL_PID, info.modid, 0, ksceLcdSetBrightness_addr, (uintptr_t *)&ksceLcdSetBrightness);
  if (res_offset2 < 0) {
    LOG("[LCD] module_get_offset(\"ksceLcdSetBrightness\"): 0x%08X\n", res_offset2);
  }

  if (ksceLcdGetBrightness != NULL && ksceLcdSetBrightness != NULL && res_offset1 >= 0 &&
      res_offset2 >= 0) {
    // Note: I'm calling by offset instead of importing them
    // because importing LCD module on OLED device prevents vitabright from
    // loading
    ksceLcdSetBrightness(ksceLcdGetBrightness());
  }

  lcd_set_brightness_hook = taiHookFunctionExportForKernel(KERNEL_PID,
                                                           &lcd_set_brightness_ref,
                                                           "SceLcd",
                                                           TAI_ANY_LIBRARY,
                                                           0x581D3A87,
                                                           hook_ksceLcdSetBrightness);
  if (lcd_set_brightness_hook < 0) {
    LOG("[LCD] taiHookFunctionExportForKernel: 0x%08X, abandoning...\n", lcd_set_brightness_hook);
  }
}

void lcd_disable_hooks() {
  if (lcd_table_inject >= 0)
    taiInjectReleaseForKernel(lcd_table_inject);

  if (lcd_set_brightness_hook >= 0)
    taiHookReleaseForKernel(lcd_set_brightness_hook, lcd_set_brightness_ref);
}
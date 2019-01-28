#include "hooks.h"
#include "../log.h"
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

// Required in order to jump to code that is in thumb mode
#define THUMB_BIT 1

static SceUID lcd_table_inject = -1;

// static uint8_t old_brightness_values[17] = {31,  37,  43,  50,  58,  67,
//                                             77,  88,  100, 114, 129, 147,
//                                             166, 182, 203, 227, 255};

// Generated using build_values.py
static uint8_t lcd_brightness_values[17] = {
    5, 9, 14, 22, 30, 41, 52, 66, 81, 97, 115, 134, 155, 178, 202, 227, 255,
};

int (*ksceLcdGetBrightness)();
int (*ksceLcdSetBrightness)(unsigned int brightness);

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset,
                      uintptr_t *addr);

int ksceKernelSysrootGetSystemSwVersion(void);

void lcd_enable_hooks() {
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  int ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceLcd", &info);
  LOG("[LCD] getmodninfo: 0x%08X\n", ret);
  LOG("[LCD] modid: 0x%08X\n", info.modid);

  if (ret < 0)
    return;

  tai_module_info_t shell_info;
  shell_info.size = sizeof(shell_info);

  uint32_t lcd_table_off = 0;
  size_t ksceLcdGetBrightness_addr = 0;
  size_t ksceLcdSetBrightness_addr = 0;

  unsigned int sw_version = ksceKernelSysrootGetSystemSwVersion();
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
    LOG("[OLED] Unsupported OS version: 0x%08X\n", sw_version);
    return;
  }

  lcd_table_inject = taiInjectDataForKernel(
      KERNEL_PID, info.modid, 0, lcd_table_off, // 0x1BA0?
      lcd_brightness_values, sizeof(lcd_brightness_values));
  LOG("[LCD] injectdata: 0x%08X\n", ret);

  int res_offset1 =
      module_get_offset(KERNEL_PID, info.modid, 0, ksceLcdGetBrightness_addr,
                        (uintptr_t *)&ksceLcdGetBrightness);
  if (res_offset1 < 0) {
    LOG("[LCD] module_get_offset1: 0x%08X\n", res_offset1);
  }

  int res_offset2 =
      module_get_offset(KERNEL_PID, info.modid, 0, ksceLcdSetBrightness_addr,
                        (uintptr_t *)&ksceLcdSetBrightness);
  if (res_offset2 < 0) {
    LOG("[LCD] module_get_offset2: 0x%08X\n", res_offset2);
  }

  if (ksceLcdGetBrightness != NULL && ksceLcdSetBrightness != NULL &&
      res_offset1 >= 0 && res_offset2 >= 0) {
    // Note: I'm calling by offset instead of importing them
    // because importing LCD module on OLED device prevents module from
    // loading
    ksceLcdSetBrightness(ksceLcdGetBrightness());
  }
}

void lcd_disable_hooks() {
  if (lcd_table_inject >= 0)
    taiInjectReleaseForKernel(lcd_table_inject);
}
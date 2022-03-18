#include "hooks.h"
#include "../main.h"
#include "../log.h"
#include "lut.h"
#include "parser.h"
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <taihen.h>

// Required in order to jump to code that is in thumb mode
#define THUMB_BIT 1

static SceUID power_set_max_brightness_hook = -1;
static tai_hook_ref_t power_set_max_brightness_ref = 0;

unsigned char lookupNew[LUT_SIZE] = {0};
static SceUID lut_inject = -1;
static SceUID oled_set_brightness_hook = -1;

int (*ksceOledGetBrightness)() = NULL;
int (*ksceOledSetBrightness)(unsigned int brightness) = NULL;
int (*ksceOledGetDDB)(uint16_t *supplier_id, uint16_t *supplier_elective_data) = NULL;

static tai_hook_ref_t oled_set_brightness_ref = -1;

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

int oled_apply_lut();

int isDimmingWorkAround = 1;

int hook_ksceOledSetBrightness(unsigned int brightness) {
  // Trying to dim screen after inactivity
  if (brightness == 1) {
    int old_level = ksceOledGetBrightness();

    // Corresponds to the line of screen dimmed after inactivity
    if (isDimmingWorkAround && old_level <= 4*0x1000) {
      // Do nothing because it would increase the brightness
      return TAI_CONTINUE(int, oled_set_brightness_ref, old_level);
    }
  }

  return TAI_CONTINUE(int, oled_set_brightness_ref, brightness);
}

int hook_kscePowerSetDisplayMaxBrightnessForOled(int limit) {
  // Workaround for https://github.com/yifanlu/taiHEN/issues/12
  if (power_set_max_brightness_ref == 0) {
    return 0;
  }

  // Request to limit the brightness slider below the max.
  if (limit < 0x10000 && limit >= 0) {
    // Exclude the last two brightness levels on max performance mode because the PS Vita crashes.
    limit = 0x10000 - 2*0x1000;
  } else {
    limit = 0x10000;
  }

  return TAI_CONTINUE(int, power_set_max_brightness_ref, limit);
}

void oled_enable_hooks() {
  int ret = parse_lut(lookupNew);
  if (ret < 0) {
    LOG("[OLED] brightness table parsing failure\n");
    return;
  }

  oled_apply_lut();
}

int oled_apply_lut() {
  // Reduce max brightness limit when PS Vita is running in power mode C/D.
  // Don't entirely remove this limit because the PS Vita crashes on the two highest levels.
  power_set_max_brightness_hook = taiHookFunctionExportForKernel(KERNEL_PID, &power_set_max_brightness_ref, "ScePower", TAI_ANY_LIBRARY, 0x77027B6B, hook_kscePowerSetDisplayMaxBrightnessForOled);
  LOG("[OLED] hooking kscePowerSetDisplayMaxBrightness: 0x%08X\n", power_set_max_brightness_hook);

  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  int ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceOled", &info);
  LOG("[OLED] getmodninfo(\"SceOled\"): 0x%08X\n", ret);
  LOG("[OLED] SceOled modid: 0x%08X\n", info.modid);

  if (ret < 0) {
    LOG("[OLED] Couldn't get SceOled module id! Abandoning...\n");
    return ret;
  }

  if (sizeof(lookupNew) != LUT_SIZE) {
    LOG("[OLED] size mismatch! Abandoning...\n");
    return -1;
  }

  LOG("[OLED] Size ok, hooking...\n");

  uint32_t oled_lut_off = 0;
  size_t ksceOledGetBrightness_addr = 0;
  size_t ksceOledSetBrightness_addr = 0;
  size_t ksceOledGetDDB_addr = 0;

  switch (sw_version >> 16) {
  case 0x360:
  case 0x365:
  case 0x367:
  case 0x368:
  case 0x369:
  case 0x370: {
    ksceOledGetBrightness_addr = 0x12BC | THUMB_BIT;
    ksceOledSetBrightness_addr = 0x0F44 | THUMB_BIT;
    ksceOledGetDDB_addr = 0x054c | THUMB_BIT;
    break;
  }
  default: // Not supported
    LOG("[OLED] Unsupported OS version: 0x%08X\n", (unsigned int)sw_version);
    return -2;
  }

  LOG("OS version: 0x%08X\nksceOledGetBrightness_addr: 0x%08X\n"
    "[OLED] ksceOledSetBrightness_addr: 0x%08X\n[OLED] ksceOledGetDDB_addr: 0x%08X\n",
      sw_version,
      ksceOledGetBrightness_addr,
      ksceOledSetBrightness_addr,
      ksceOledGetDDB_addr);

  int res_offset1 = module_get_offset(
      KERNEL_PID, info.modid, 0, ksceOledGetBrightness_addr, (uintptr_t *)&ksceOledGetBrightness);
  if (res_offset1 < 0) {
    LOG("[OLED] module_get_offset(\"ksceOledGetBrightness\"): 0x%08X\n", res_offset1);
  }

  int res_offset2 = module_get_offset(
      KERNEL_PID, info.modid, 0, ksceOledSetBrightness_addr, (uintptr_t *)&ksceOledSetBrightness);

  if (res_offset2 < 0) {
    LOG("[OLED] module_get_offset(\"ksceOledSetBrightness\"): 0x%08X\n", res_offset1);
  }
  
  int res_offset3 = module_get_offset(
    KERNEL_PID, info.modid, 0, ksceOledGetDDB_addr, (uintptr_t*)&ksceOledGetDDB);

  if (res_offset3 < 0) {
    LOG("[OLED] module_get_offset3: 0x%08X\n", res_offset3);
  }

  if (ksceOledGetBrightness != NULL && ksceOledSetBrightness != NULL && ksceOledGetDDB != NULL && res_offset1 >= 0 &&
      res_offset2 >= 0 && res_offset3 >= 0) {
    
    // Get table offset for type
    uint16_t supplier_id = 0;
    uint16_t supplier_elective_data = 0;
    ret = ksceOledGetDDB(&supplier_id, &supplier_elective_data);
    if (ret < 0) {
      LOG("[OLED] cannot get DDB: 0x%08X\n", ret);
    } else {
      LOG("[OLED] supplier_id: 0x%04X, supplier_elective_data: 0x%04X\n", supplier_id, supplier_elective_data);
    }

    switch(supplier_elective_data & 0xFF) {
      case 4:
        oled_lut_off = 0x1AB8;
        break;
      case 5:
        oled_lut_off = 0x1C20;
        break;
      default:
        oled_lut_off = 0x1E00;
        break;
    }

    LOG("[OLED] LUT table start address: 0x%08X\n",
        (unsigned int)oled_lut_off);

    lut_inject =
      taiInjectDataForKernel(KERNEL_PID, info.modid, 0, oled_lut_off, lookupNew, sizeof(lookupNew));
    LOG("[OLED] injectdata: 0x%08X\n", lut_inject);
    
    // Note: I'm calling by offset instead of importing them
    // because importing OLED module on LCD devices prevents vitabright from loading
    ksceOledSetBrightness(ksceOledGetBrightness());
  }

  oled_set_brightness_hook = taiHookFunctionExportForKernel(KERNEL_PID,
                                                            &oled_set_brightness_ref,
                                                            "SceOled",
                                                            TAI_ANY_LIBRARY,
                                                            0xF9624C47,
                                                            hook_ksceOledSetBrightness);
  if (oled_set_brightness_hook < 0) {
    LOG("[OLED] taiHookFunctionExportForKernel: 0x%08X\n", oled_set_brightness_hook);
  }

  return 0;
}

void oled_disable_hooks() {
  if (lut_inject >= 0)
    taiInjectReleaseForKernel(lut_inject);

  if (oled_set_brightness_hook >= 0)
    taiHookReleaseForKernel(oled_set_brightness_hook, oled_set_brightness_ref);
}

/*
 * Syscall exports.
 */

int vitabrightOledGetLevel() {
  int state;
  ENTER_SYSCALL(state);
  int brightness = ksceOledGetBrightness();

  int level;
  if (brightness == 0) {
    level = -1;
  } else if (brightness == 1) {
    level = 16;
  } else if (brightness < 0x1000) {
    level = 15;
  } else if (brightness >= 0x10000) {
    level = 0;
  } else {
    level = 16 - ((brightness + 0x1000) / 0x1000);
  }

  EXIT_SYSCALL(state);
  return level;
}

int vitabrightOledSetLevel(unsigned int level) {
  int state;
  ENTER_SYSCALL(state);

  unsigned int brightness = 0;
  if (level > 16) {
    brightness = 0;
  } else if (level == 16) {
    brightness = 1;
  } else if (level == 15) {
    brightness = 0xfff;
  } else if (level == 0) {
    brightness = 0x10000;
  } else {
    brightness = 0x1000 * (16 - level) - 0x1000;
  }

  isDimmingWorkAround = 0;
  ksceOledSetBrightness(brightness);
  isDimmingWorkAround = 1;

  EXIT_SYSCALL(state);
  return level;
}

int vitabrightOledGetLut(unsigned char oledLut[LUT_SIZE]) {
  int state;
  ENTER_SYSCALL(state);
  ksceKernelMemcpyKernelToUser((void *)oledLut, lookupNew, LUT_SIZE);

  EXIT_SYSCALL(state);
  return 0;
}

int vitabrightOledSetLut(unsigned char oledLut[LUT_SIZE]) {
  int state;
  ENTER_SYSCALL(state);

  oled_disable_hooks();
  ksceKernelMemcpyUserToKernel(lookupNew, (void *)oledLut, LUT_SIZE);

  oled_apply_lut();

  EXIT_SYSCALL(state);
  return 0;
}

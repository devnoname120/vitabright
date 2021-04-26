#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

#include "lut.h"
#include "parser.h"
#include "custom_lut.h"

static uint32_t fwv = 0;
static uint8_t hasLCD = 0;
static uint32_t prevLUT = 0;

static SceUID lcd_brightness_patchID = 0;
static uint8_t lcd_brightness_values[17] = { 1, 3, 5, 8, 13, 20, 29, 41, 57, 76, 95, 116, 137, 161, 190, 220, 255 };

static int (*getBrightness)() = NULL;
static int (*setBrightness)(int b) = NULL;

static SceUID sceDisplayForDriver_9E3C6DC6_patch = 0;
static tai_hook_ref_t sceDisplayForDriver_9E3C6DC6_hook;
int sceDisplayForDriver_9E3C6DC6_custom(int target, int value) {
  if (value == 1)
    return 0;
  return TAI_CONTINUE(int, sceDisplayForDriver_9E3C6DC6_hook, target, value);
}

static int patchDisplay(int patch) {
  int ret;
  if (patch)
    ret = sceDisplayForDriver_9E3C6DC6_patch = taiHookFunctionExportForKernel(KERNEL_PID, &sceDisplayForDriver_9E3C6DC6_hook, "SceDisplay", TAI_ANY_LIBRARY, 0x9E3C6DC6, sceDisplayForDriver_9E3C6DC6_custom);
  else
    ret = taiHookReleaseForKernel(sceDisplayForDriver_9E3C6DC6_patch, sceDisplayForDriver_9E3C6DC6_hook);
  ksceDebugPrintf("[VBRIGHT] sceDisplay %s: 0x%X\n", (patch) ? "patch" : "unpatch", ret);
  return ret;
}

static int patchLCD(int patch) {
  int ret;
  if (patch)
    ret = lcd_brightness_patchID = taiInjectDataForKernel(KERNEL_PID, ksceKernelSearchModuleByName("SceLcd"), 0, (fwv < 0x03630000) ? 0x1B00 : 0x1B48, lcd_brightness_values, sizeof(lcd_brightness_values));
  else
    ret = taiInjectReleaseForKernel(lcd_brightness_patchID);
  ksceDebugPrintf("[VBRIGHT] sceLcd %s: 0x%X\n", (patch) ? "patch" : "unpatch", ret);
  return ret;
}

static int patchOLED(int patch) {
  if (parse_lut(customLUT) < 0)
    ksceDebugPrintf("[VBRIGHT] Error parsing custom LUT\n");
  int ret = -1;
  uintptr_t sceOled_start = 0;
  if (module_get_offset(KERNEL_PID, ksceKernelSearchModuleByName("SceOled"), 0, 0, &sceOled_start) >= 0 && sceOled_start) {
    DACR_OFF(
      if (patch) {
        *(uint32_t*)(sceOled_start + 0xbd0) = (uint32_t)0xbf00611a; // default
        *(uint32_t*)(sceOled_start + 0xc70) = (uint32_t)0xbf00611a; // type 5
        *(uint16_t*)(sceOled_start + 0xcb8) = (uint16_t)0xbf00; // type 4
      } else {
        *(uint32_t*)(sceOled_start + 0xbd0) = (uint32_t)0x1203e9c3;
        *(uint32_t*)(sceOled_start + 0xc70) = (uint32_t)0x1203e9c3;
        *(uint16_t*)(sceOled_start + 0xcb8) = (uint32_t)0x60da;
      }
    );
    if (module_get_offset(KERNEL_PID, ksceKernelSearchModuleByName("SceOled"), 1, 0, &sceOled_start) >= 0 && sceOled_start) {
      DACR_OFF(
        if (patch) {
          prevLUT = *(uint32_t*)(sceOled_start + 0x2c);
          *(uint32_t*)(sceOled_start + 0x2c) = (uint32_t)customLUT;
        } else if (prevLUT)
            *(uint32_t*)(sceOled_start + 0x2c) = (uint32_t)prevLUT;
      );
      ret = 0;
    }
  }
  ksceDebugPrintf("[VBRIGHT] sceOled %s: 0x%X\n", (patch) ? "patch" : "unpatch", ret);
  return ret;
}

static int getDriverOffsets() {
  module_get_export_func(KERNEL_PID, (hasLCD) ? "SceLcd" : "SceOled", TAI_ANY_LIBRARY, (hasLCD) ? 0x581D3A87 : 0xF9624C47, &setBrightness);
  module_get_export_func(KERNEL_PID, (hasLCD) ? "SceLcd" : "SceOled", TAI_ANY_LIBRARY, (hasLCD) ? 0x3A6D6AC3 : 0x43EF811A, &getBrightness);
  if (!setBrightness || !getBrightness)
    return -1;
  return 0;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {

  fwv = *(uint32_t*)(*(int*)(ksceSysrootGetSysbase() + 0x6c) + 0x4);
  hasLCD = *(uint8_t*)(*(int*)(ksceSysrootGetSysbase() + 0x6c) + 0xe8) & 9;
  
  ksceDebugPrintf("[VBRIGHT] Hello from VitaBright!\n[VBRIGHT] fw: 0x%08X | display: %s\n", fwv, (hasLCD) ? "LCD" : "OLED");
  
  if (ksceKernelSearchModuleByName("SceHdmi") >= 0)
    return SCE_KERNEL_START_FAILED;
  
  if (fwv < 0x03600000 || fwv > 0x03730011)
    return SCE_KERNEL_START_FAILED;
  
  if (patchDisplay(1) < 0)
    return SCE_KERNEL_START_FAILED;
  
  if (((hasLCD) ? patchLCD(1) : patchOLED(1)) < 0)
    return SCE_KERNEL_START_FAILED;
  
  if (getDriverOffsets() < 0)
    return SCE_KERNEL_START_FAILED;
  
  setBrightness(getBrightness());
  
  ksceDebugPrintf("[VBRIGHT] Init done!\n");

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  if (hasLCD)
    patchLCD(0);
  else
    patchOLED(0);
  return SCE_KERNEL_STOP_SUCCESS;
}

// EXPORTS
int vitabrightReload() {
  int state;
  ENTER_SYSCALL(state);

  if (hasLCD) {
    patchLCD(0);
    patchLCD(1);
  } else {
    patchOLED(0);
    patchOLED(1);
  }

  EXIT_SYSCALL(state);

  return 0;
}

int vitabrightOledGetLevel() {
  int state;
  ENTER_SYSCALL(state);
  int brightness = getBrightness();

  int level;
  if (brightness == 0)
    level = -1;
  else if (brightness == 1)
    level = 16;
  else if (brightness < 0x1000)
    level = 15;
  else if (brightness >= 0x10000)
    level = 0;
  else
    level = 16 - ((brightness + 0x1000) / 0x1000);

  EXIT_SYSCALL(state);
  return level;
}

int vitabrightOledSetLevel(unsigned int level) {
  int state;
  ENTER_SYSCALL(state);

  unsigned int brightness = 0;
  if (level > 16)
    brightness = 0;
  else if (level == 16)
    brightness = 1;
  else if (level == 15)
    brightness = 0xfff;
  else if (level == 0)
    brightness = 0x10000;
  else
    brightness = 0x1000 * (16 - level) - 0x1000;

  setBrightness(brightness);

  EXIT_SYSCALL(state);
  return level;
}

int vitabrightOledGetLut(unsigned char oledLut[LUT_SIZE]) {
  int state;
  ENTER_SYSCALL(state);
  ksceKernelMemcpyKernelToUser((uintptr_t)oledLut, customLUT, LUT_SIZE);
  EXIT_SYSCALL(state);
  return 0;
}

int vitabrightOledSetLut(unsigned char oledLut[LUT_SIZE]) {
  int state;
  ENTER_SYSCALL(state);
  ksceKernelMemcpyUserToKernel(customLUT, (uintptr_t)oledLut, LUT_SIZE);
  setBrightness(getBrightness());
  EXIT_SYSCALL(state);
  return 0;
}

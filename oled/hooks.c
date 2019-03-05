#include "hooks.h"
#include "../log.h"
#include "lut.h"
#include "parser.h"
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

// Required in order to jump to code that is in thumb mode
#define THUMB_BIT 1

static unsigned char lookupNew[LUT_SIZE] = {0};
static SceUID lut_inject = -1;

int (*ksceOledGetBrightness)() = NULL;
int (*ksceOledSetBrightness)(unsigned int brightness) = NULL;

static tai_hook_ref_t oled_set_brightness_ref = -1;

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset,
                      uintptr_t *addr);

int ksceKernelSysrootGetSystemSwVersion(void);


int hook_ksceOledSetBrightness(unsigned int brightness) {
  // Trying to dim screen after inactivity
  if (brightness == 1) {
    int old_level = ksceOledGetBrightness();

    // HACK: In modified vitabright_lut.txt, it corresponds to the line of screen dimmed after inactivity
    if (old_level <= 16384) {
      // Do nothing because it would increase brightness
      return TAI_CONTINUE(int, oled_set_brightness_ref, old_level);
    }
  }

  return TAI_CONTINUE(int, oled_set_brightness_ref, brightness);
}

void oled_enable_hooks() {
  int ret = parse_lut(lookupNew);
  if (ret < 0) {
    LOG("[OLED] brightness table parsing failure\n");
    return;
  }

  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceOled", &info);
  LOG("[OLED] getmodninfo: 0x%08X\n", ret);
  LOG("[OLED] modid: 0x%08X\n", info.modid);

  if (ret < 0)
    return;

  if (sizeof(lookupNew) != LUT_SIZE) {
    LOG("[OLED] size mismatch! Skipping...\n");
    return;
  }

  LOG("[OLED] Size ok, hooking...\n");

  uint32_t oled_lut_off = 0;
  size_t ksceOledGetBrightness_addr = 0;
  size_t ksceOledSetBrightness_addr = 0;

  unsigned int sw_version = ksceKernelSysrootGetSystemSwVersion();
  switch (sw_version >> 16) {
  case 0x360:
  case 0x365:
  case 0x367:
  case 0x368:
  case 0x369:
  case 0x370: {
    oled_lut_off = 0x1E00;
    ksceOledGetBrightness_addr = 0x12BC | THUMB_BIT;
    ksceOledSetBrightness_addr = 0x0F44 | THUMB_BIT;
    break;
  }
  default: // Not supported
    LOG("[OLED] Unsupported OS version: 0x%08X\n", (unsigned int)sw_version);
    return;
  }

  LOG("[OLED] OS version: 0x%08X\n, table offset: 0x%08X, ksceOledGetBrightness_addr: 0x%08X, ksceOledSetBrightness_addr: 0x%08X\n",
  sw_version, (unsigned int)oled_lut_off, ksceOledGetBrightness_addr, ksceOledSetBrightness_addr);

  lut_inject = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, oled_lut_off,
                                      lookupNew, sizeof(lookupNew));
  LOG("[OLED] injectdata: 0x%08X\n", lut_inject);

  int res_offset1 =
      module_get_offset(KERNEL_PID, info.modid, 0, ksceOledGetBrightness_addr,
                        (uintptr_t *)&ksceOledGetBrightness);
  if (res_offset1 < 0) {
    LOG("[OLED] module_get_offset1: 0x%08X\n", res_offset1);
  }

  int res_offset2 =
      module_get_offset(KERNEL_PID, info.modid, 0, ksceOledSetBrightness_addr,
                        (uintptr_t *)&ksceOledSetBrightness);

  if (res_offset2 < 0) {
    LOG("[OLED] module_get_offset2: 0x%08X\n", res_offset2);
  }

  if (ksceOledGetBrightness != NULL && ksceOledSetBrightness != NULL &&
      res_offset1 >= 0 && res_offset2 >= 0) {
    // Note: I'm calling by offset instead of importing them
    // because importing OLED module on LCD devices prevents vitabright from loading
    ksceOledSetBrightness(ksceOledGetBrightness());
  }

  int res_hook = taiHookFunctionExportForKernel(KERNEL_PID, &oled_set_brightness_ref, "SceOled", TAI_ANY_LIBRARY, 0xF9624C47, hook_ksceOledSetBrightness);
  if (res_hook < 0) {
    LOG("[OLED] taiHookFunctionExportForKernel: 0x%08X\n", res_hook);
  }
}

void oled_disable_hooks() {
  if (lut_inject >= 0)
    taiInjectReleaseForKernel(lut_inject);
}
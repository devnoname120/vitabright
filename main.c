#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <taihen.h>

#include "lcd/hooks.h"
#include "log.h"
#include "oled/hooks.h"

int ksceDisplaySetBrightness(int unk, unsigned int brightness);

static tai_hook_ref_t hook_power_get_max_brightness = 0;

unsigned int power_get_max_brightness() {
  if (hook_power_get_max_brightness != 0) {
    return TAI_CONTINUE(unsigned int, hook_power_get_max_brightness);
  }

  return 65536;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
  LOG("vitabright started...\n");

  // int ret = taiHookFunctionExportForKernel(KERNEL_PID,
  // &hook_power_get_max_brightness,
  // 							 "ScePower",
  // 							 TAI_ANY_LIBRARY,
  // 							 0xD8759B55,
  // 							 power_get_max_brightness);
  // LOG("ret: 0x%08X\n", ret);

  oled_enable_hooks();
  lcd_enable_hooks();

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  oled_disable_hooks();
  lcd_disable_hooks();

  return SCE_KERNEL_STOP_SUCCESS;
}

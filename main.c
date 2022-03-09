#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

#include "lcd/hooks.h"
#include "log.h"
#include "oled/hooks.h"

int ksceDisplaySetBrightness(int unk, unsigned int brightness);

static SceUID power_set_max_brightness_hook = -1;
static tai_hook_ref_t power_set_max_brightness_ref = 0;

int hook_kscePowerSetDisplayMaxBrightness(int limit) {
  // Workaround for https://github.com/yifanlu/taiHEN/issues/12
  if (power_set_max_brightness_ref == 0) {
    return 0;
  }

  return TAI_CONTINUE(int, power_set_max_brightness_ref, 0x10000);
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
  LOG("vitabright started...\n");

  // Remove max brightness limit when PS Vita is running in power mode C/D.
  power_set_max_brightness_hook = taiHookFunctionExportForKernel(KERNEL_PID, &power_set_max_brightness_ref, "ScePower", TAI_ANY_LIBRARY, 0x77027B6B, hook_kscePowerSetDisplayMaxBrightness);
  LOG("[display] taiHookFunctionExportForKernel: 0x%08X\n", power_set_max_brightness_hook);

  oled_enable_hooks();
  lcd_enable_hooks();

  return SCE_KERNEL_START_SUCCESS;
}

// Exported as syscall.
int vitabrightReload() {
  int state;
  ENTER_SYSCALL(state);

  oled_disable_hooks();
  lcd_disable_hooks();

  oled_enable_hooks();
  lcd_enable_hooks();

  EXIT_SYSCALL(state);

  return 0;
}

int module_stop(SceSize argc, const void *args) {
  oled_disable_hooks();
  lcd_disable_hooks();

  if (power_set_max_brightness_hook >= 0)
    taiHookReleaseForKernel(power_set_max_brightness_hook, power_set_max_brightness_ref);

  return SCE_KERNEL_STOP_SUCCESS;
}

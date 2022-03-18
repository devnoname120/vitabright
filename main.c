#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>
#include <psp2kern/kernel/sysroot.h>

#include "main.h"
#include "lcd/hooks.h"
#include "log.h"
#include "oled/hooks.h"

static SceUID power_set_max_brightness_hook = -1;
static tai_hook_ref_t power_set_max_brightness_ref = 0;

unsigned int sw_version = 0;

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
  LOG("vitabright started...\n");

  sw_version = ksceKernelSysrootGetSystemSwVersion();
  LOG("[OS] version: %08X\n", sw_version);

  // Boot type indicator 1. See https://wiki.henkaku.xyz/vita/Sysroot#Boot_type_indicator_1
  int is_lcd = *(uint8_t*)(ksceKernelSysrootGetKblParam() + 0xE8) & 9;
  LOG("[OS] isLcd: %d", is_lcd);

  if (is_lcd){
    lcd_enable_hooks();
  } else {
    oled_enable_hooks();
  }

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

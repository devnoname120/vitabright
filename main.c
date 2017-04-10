#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include "log.h"

void _start() __attribute__ ((weak, alias ("module_start")));
	
static tai_hook_ref_t hook_get_max_brightness;
static tai_hook_ref_t hook_set_brightness;

int get_max_brightness(uint *brightness)
{
	int ret = TAI_CONTINUE(int, hook_get_max_brightness, brightness);

	uint brightness_ker;
	ksceKernelMemcpyUserToKernel(&brightness_ker, brightness, sizeof(uint));
	
	LOG("brightness = %u, get_brightness() = %d\n", brightness_ker, ret);
	return ret;
}

int set_brightness(uint brightness)
{
	// Min: 21, Max: 65536
	uint mod_brightness = (brightness-15)*(65536)/(65536-15);
	//uint mod_brightness = (brightness + 10000); 
	int ret = TAI_CONTINUE(uint, hook_set_brightness, mod_brightness);
	LOG("set_brightness(%u) = %d, corrected to %u\n", brightness, ret, mod_brightness);
	return ret;
}


int module_start(SceSize argc, const void *args)
{
	log_reset();
	
	LOG("vitabright started...\n");	

	int ret = taiHookFunctionExportForKernel(KERNEL_PID, &hook_get_max_brightness,
                                 "SceAVConfig",
                                 TAI_ANY_LIBRARY,
                                 0x6ABA67F4,
                                 get_max_brightness);
	LOG("ret: 0x%08X\n", ret);

	ret = taiHookFunctionExportForKernel(KERNEL_PID, &hook_set_brightness,
                                 "SceAVConfig",
                                 TAI_ANY_LIBRARY,
                                 0xE0C1B743,
                                 set_brightness);
	
	LOG("ret: 0x%08X\n", ret);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

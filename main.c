#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include "log.h"
#include "lookupNew.h"


int ksceDisplaySetBrightness(int unk, unsigned int brightness);

//static tai_hook_ref_t hook_get_max_brightness;
static tai_hook_ref_t hook_set_brightness;

/*int get_max_brightness(uint *brightness)
{
	int ret = TAI_CONTINUE(int, hook_get_max_brightness, brightness);

	uint brightness_ker;
	ksceKernelMemcpyUserToKernel(&brightness_ker, brightness, sizeof(uint));
	
	//LOG("brightness = %u, get_brightness() = %d\n", brightness_ker, ret);
	return ret;
}*/

int set_brightness(uint brightness)
{
	uint mod_brightness;
	
	if (brightness < 100)
		mod_brightness = 1;
	else
		mod_brightness = (brightness-19)*(65536)/(65536-19);
	//int ret = TAI_CONTINUE(uint, hook_set_brightness, mod_brightness);

	LOG("set_brightness(%u), corrected to %u\n", brightness, mod_brightness);
	
	ksceDisplaySetBrightness(0, mod_brightness);
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	LOG("vitabright started...\n");	

	/*ret = taiHookFunctionExportForKernel(KERNEL_PID, &hook_get_max_brightness,
                                 "SceAVConfig",
                                 TAI_ANY_LIBRARY,
                                 0x6ABA67F4,
                                 get_max_brightness);
	LOG("ret: 0x%08X\n", ret);
	*/
	int ret = taiHookFunctionExportForKernel(KERNEL_PID, &hook_set_brightness,
                                 "SceAVConfig",
                                 TAI_ANY_LIBRARY,
                                 0xE0C1B743,
                                 set_brightness);
	LOG("hook set_brightness: 0x%08X\n", ret);
	
	tai_module_info_t info;
	info.size = sizeof(tai_module_info_t);
	ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceOled", &info);
	LOG("getmodninfo: 0x%08X\n", ret);
	LOG("modid: 0x%08X\n", info.modid);
	
	if (sizeof(lookupNew) != 357) {
		LOG("size mismatch! Skipping...\n");
		return SCE_KERNEL_START_SUCCESS;
	}

	LOG("Size ok, hooking...\n");
	ret = taiInjectDataForKernel(KERNEL_PID, info.modid, 1, 0x1E00, lookupNew, sizeof(lookupNew));	
	LOG("injectdata: 0x%08X\n", ret);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include "lut.h"
#include "parser.h"
#include "log.h"

unsigned char lookupNew[LUT_SIZE] = {0};

int ksceOledGetBrightness();
int ksceOledSetBrightness(unsigned int brightness);
int ksceDisplaySetBrightness(int unk, unsigned int brightness);

static tai_hook_ref_t hook_power_get_max_brightness = 0;
SceUID lut_inject = -1;

unsigned int power_get_max_brightness()
{
	if (hook_power_get_max_brightness != 0) {
		return TAI_CONTINUE(unsigned int, hook_power_get_max_brightness);
	}
	
	return 65536;
}


void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args)
{
	LOG("vitabright started...\n");

	// int ret = taiHookFunctionExportForKernel(KERNEL_PID, &hook_power_get_max_brightness,
	// 							 "ScePower",
	// 							 TAI_ANY_LIBRARY,
	// 							 0xD8759B55,
	// 							 power_get_max_brightness);
	// LOG("ret: 0x%08X\n", ret);
	

	int ret = parse_lut(lookupNew);
	if (ret < 0)
	{
		LOG("brightness table parsing failure\n");
		return ret;
	}

	tai_module_info_t info;
	info.size = sizeof(tai_module_info_t);
	ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceOled", &info);
	LOG("getmodninfo: 0x%08X\n", ret);
	LOG("modid: 0x%08X\n", info.modid);

	if (sizeof(lookupNew) != LUT_SIZE)
	{
		LOG("size mismatch! Skipping...\n");
		return SCE_KERNEL_START_SUCCESS;
	}

	LOG("Size ok, hooking...\n");
	lut_inject = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0x1E00, lookupNew, sizeof(lookupNew));
	LOG("injectdata: 0x%08X\n", ret);

	ksceOledSetBrightness(ksceOledGetBrightness());

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	if (lut_inject >= 0)
		taiInjectReleaseForKernel(lut_inject);

	return SCE_KERNEL_STOP_SUCCESS;
}

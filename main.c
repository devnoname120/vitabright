#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include "log.h"

void _start() __attribute__ ((weak, alias ("module_start")));
	
//static tai_hook_ref_t hook_get_max_brightness;
static tai_hook_ref_t hook_set_brightness;

#define DEVICE_BASE         (0xE0000000)
#define IFTU0_BASE (DEVICE_BASE + 0x5020000)


uint *iftu0_base = NULL;

static inline void turnon_oled(int a2, int a3)
{
    if (a2 == 128)
    {
        *(iftu0_base + 0x2000 + 32) = 0;
        *(iftu0_base + 0x2000 + 4) &= ~1;
    }
    else
    {
        *(iftu0_base + 0x2000 + 32) = a2;
        *(iftu0_base + 0x2000 + 4) |= 1;
    }

    if (a3 == 256)
    {
        *(iftu0_base + 0x1000 + 160) = 1;
    }
    else
    {
        *(iftu0_base + 0x1000 + 140) = a3;
        *(iftu0_base + 0x1000 + 160) = 0;
    }
}

int brightness_adjust(void)
{
    for (int j = 0; j < 256; j += 2)
    {
        turnon_oled(4, j);
        for (int k = 0; k < 0x1000000; k++);
    }
    turnon_oled(128, 0);
    for (int j = 254; j >= 0; j -= 2)
    {
        turnon_oled(4, j);
        for (int k = 0; k < 0x1000000; k++);
    }
}



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
	
	//int intensity = (char)((brightness-20)*256/(65536-20));
	LOG("set_brightness(%u), corrected to %u\n", brightness, mod_brightness);
	
	SceDisplayForDriver_0x9E3C6DC6(0, mod_brightness);

	//brightness_adjust();
	//turnon_oled(4, intensity);

	return 0;
}


int module_start(SceSize argc, const void *args)
{
	/*SceKernelAllocMemBlockKernelOpt opt = {0};
	opt.size = sizeof(opt);
	opt.attr = 2;
	opt.paddr = IFTU0_BASE;
	int payload_block = ksceKernelAllocMemBlock("hw_regs", 0x20100206, 0x3000, &opt); // Need 0x2032 but it has to be page-aligned so 0x3000
	if (payload_block < 0)
		LOG("AllocMemBlock ret=0x%08X", payload_block);
	int ret = ksceKernelGetMemBlockBase(payload_block, (void**)&iftu0_base);
	
	if (ret < 0)
		LOG("GetMemBlock ret=0x%08X", ret);
		
	*/
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
	
	LOG("ret: 0x%08X\n", ret);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

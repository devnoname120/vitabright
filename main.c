#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include "log.h"

#define LUT_FILE1 "ur0:tai/vitabright_lut.txt"
#define LUT_FILE2 "ux0:tai/vitabright_lut.txt"
#define LUT_SIZE (357)
#define LUT_LINE_SIZE (21)

unsigned char lookupNew[LUT_SIZE] = {0};

int ksceDisplaySetBrightness(int unk, unsigned int brightness);

//static tai_hook_ref_t hook_get_max_brightness;
SceUID lut_inject = -1;
static tai_hook_ref_t hook_set_brightness;

/*int get_max_brightness(uint *brightness)
{
	int ret = TAI_CONTINUE(int, hook_get_max_brightness, brightness);

	uint brightness_ker;
	ksceKernelMemcpyUserToKernel(&brightness_ker, brightness, sizeof(uint));

	//LOG("brightness = %u, get_brightness() = %d\n", brightness_ker, ret);
	return ret;
}*/

int is_hex(unsigned char c)
{
	return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F');
}

int parse_hex_digit(unsigned char c)
{
	if (c < 'A')
	{
		return c - '0';
	}
	else
	{
		return c - 'A' + 10;
	}
}

int hex_to_int(unsigned char c[2])
{
	if (!is_hex(c[0]) || !is_hex(c[1]))
	{
		return -1;
	}

	return 16 * parse_hex_digit(c[0]) + parse_hex_digit(c[1]);
}

int parse_hex(SceUID fd)
{
	unsigned char hex_buf[2] = {0};

	int ret = ksceIoRead(fd, hex_buf, 2);
	if (ret != 2)
		return ret;

	// Note: would be easier to use strtoul but libc is annoying to use in Kernel, and I don't want to ship libk
	return hex_to_int(hex_buf);
}

int parse_line(SceUID fd, unsigned char lut_line[LUT_LINE_SIZE])
{
	for (int i = 0; i < LUT_LINE_SIZE; i++)
	{
		int val = parse_hex(fd);
		if (val < 0)
		{
			return val;
		}

		lut_line[i] = (unsigned char)val;

		char c = '\0';
		if (ksceIoRead(fd, &c, 1) != 1)
		{
			return -1;
		}

		// Space after each number, except last number from line where it's a newline character
		if (i != LUT_LINE_SIZE - 1)
		{
			if (c != ' ')
				return -1;
		}
		else
		{
			if (c != '\n')
				return -1;
		}
	}

	return 0;
}

int parse_lut()
{
	SceUID fd = ksceIoOpen(LUT_FILE1, SCE_O_RDONLY, 6);
	if (fd < 0)
	{
		fd = ksceIoOpen(LUT_FILE2, SCE_O_RDONLY, 6);

		if (fd < 0)
			return fd;
	}

	for (int l = 0; l < (LUT_SIZE / LUT_LINE_SIZE); l++)
	{
		int ret = parse_line(fd, &(lookupNew[l * LUT_LINE_SIZE]));
		if (ret < 0)
			return ret;
	}

	ksceIoClose(fd);
	return 0;
}

void _start() __attribute__((weak, alias("module_start")));
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

	int ret = parse_lut();
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

	if (sizeof(lookupNew) != 357)
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

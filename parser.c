#include <psp2kern/io/fcntl.h>
#include "lut.h"

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

int parse_lut(unsigned char lookupNew[LUT_SIZE])
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
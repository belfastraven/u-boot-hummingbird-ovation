/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <fastboot.h>
#include <asm/arch/sys_info.h>
#include <omap4_dsi.h>
#include <bn_boot.h>

#include "menu.h"
#include "console.h"

static const struct efi_partition_info partitions[] = {
	/* name       start_kb    size_kb */
	{ "xloader",       128,       128 }, /* xloader must start at 128 KB */
	{ "bootloader",      0,       256 },
	{ "recovery",        0,     15872 },
	{ "boot",      16*1024,   16*1024 },
	{ "rom",             0,   48*1024 },
	{ "bootdata",        0,   48*1024 },
	{ "factory",         0,  370*1024 },
	{ "system",          0,  612*1024 },
	{ "cache",           0,  426*1024 },
	{ "media",           0, 1024*1024 },
	{ "userdata",        0,         0 }, /* autosize to the end */
	{},
};

int mmc_slot = 1;

int fastboot_oem(const char *cmd)
{
	if (!strcmp(cmd, "format"))
		return efi_do_format(partitions, CFG_FASTBOOT_MMC_NO);
	return -1;
}

void board_mmc_init(void)
{
	/* nothing to do this early */
}

static struct omap_dsi_panel panel = {
	.xres 		= 1920,
	.yres 		= 1280,

	.dsi_data = {
		.pxl_fmt	= OMAP_PXL_FMT_RGB666_PACKED,
		.hsa		= 0,
		.hbp		= 0,
		.hfp		= 24,

		.vsa		= 1,
		.vfp 		= 10,
		.vbp		= 9,

		.window_sync	= 4,

		.regm 		= 348,
		.regn 		= 20,
		.regm_dsi 	= 8,
		.regm_dispc	= 9,
		.lp_div		= 16,
		.tl		= 1107,
		.vact		= 1280,
		.line_bufs	= 0,
		.bus_width	= 1,

		.hsa_hs_int	= 0,
		.hfp_hs_int	= 0,
		.hbp_hs_int	= 0,

		.hsa_lp_int	= 130,
		.hfp_lp_int	= 223,
		.hbp_lp_int	= 59,

		.bl_lp_int	= 0,
		.bl_hs_int	= 1038,

		.enter_lat	= 23,
		.exit_lat	= 21,

		.ths_prepare 	= 26,
		.ths_zero 	= 35,
		.ths_trail	= 26,
		.ths_exit	= 49,
		.tlpx		= 17,
		.tclk_trail	= 23,
		.tclk_zero	= 89,
		.tclk_prepare	= 22,
	},

	.dispc_data = {
		.hsw 		= 5,
		.hfp		= 4,
		.hbp		= 39,

		.vsw		= 1,
		.vfp		= 9,
		.vbp		= 10,

		.pcd		= 1,
		.lcd		= 1,
		.acbi		= 0,
		.acb		= 0,

		.row_inc	= 112,
	},
};

extern uint16_t const _binary_cyanoboot_rle_start[];
extern uint16_t const _binary_lowbatt_charge_rle_start[];

uint32_t FB = 0xb2200000;

int panel_has_enabled = 1;
int boot_displayed =0;

void disable_panel_backlight(void){

     if(panel_has_enabled)
     {
	     panel_enable(0);
	     backlight_enable(0);
     }
}

extern struct img_info bootimg_info;

int board_late_init(void)
{

	show_image(boot);

	load_serial_num();

	lcd_console_init();

	// Superhack (must read sdcard first to init the part table
	run_command("mmcinit 0; fatload mmc 0:1 0x80000000 stuff 4", 0);

	int bootmode = set_boot_mode();

	return bootmode;
}

void show_image(ppz_images image_name)
{
	int fallback = 1;
	enum omap_dispc_format fmt = OMAP_XRGB888_FMT;

	backlight_set_brightness(0x0);
	panel_enable(0);
	backlight_enable(0);
	memset(FB, 0, panel.xres * panel.yres * 2);
	display_rle(_binary_cyanoboot_rle_start, (uint16_t *) FB, panel.xres, panel.yres);

	bootimg_info.width = panel.xres;
	bootimg_info.height = panel.yres;
	bootimg_info.bg_color = 0;

	fmt = OMAP_RGB565_FMT;
	panel_enable(1);
	display_init(&panel, (void *) FB, fmt);

	backlight_enable(1);
	backlight_set_brightness(0x3f);
}

void turn_panel_off()
{
	if(panel_has_enabled) {
		backlight_enable(0);
		lp855x_restore_i2c();
		panel_has_enabled = 0;
	}
}

void turn_panel_on()
{
	if(!panel_has_enabled) {
		backlight_enable(1);
		backlight_set_brightness(0x3f);
		panel_has_enabled = 1;
	}
}

int panel_is_enabled()
{
	return panel_has_enabled;
}

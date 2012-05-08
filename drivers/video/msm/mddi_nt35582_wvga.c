/* drivers/video/msm/mddi_nt35582_wvga.c
 * Created from the original mddi_toshiba_wvga.c and mddi_nt35582.c
 *
 * Credits to CodeAurora and Huawei.
 * Check http://github.com/Blefish for my other projects.
 *
 */

/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msm_fb.h"
#include "mddihost.h"
#include <linux/pwm.h>
#ifdef CONFIG_PMIC8058_PWM
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-pwm.h>
#endif

#define BL_MAX		230

#ifdef CONFIG_PMIC8058_PWM
static struct pwm_device *bl_pwm;

#define PWM_PERIOD	1000	/* us, period of 1Khz */
#define DUTY_LEVEL	(PWM_PERIOD / BL_MAX)
#endif

static struct msm_panel_common_pdata *mddi_nt35582_pdata;

struct init_table {
	unsigned int reg;
	unsigned int val;
};

static int mddi_nt35582_panel_on(struct platform_device *pdev)
{
	int ret;
	ret = mddi_queue_register_write(0x2900, 0, 0, 0);
	ret = mddi_queue_register_write(0x1100, 0, 0, 0);
	mddi_wait(120);
	return ret;
}

static int mddi_nt35582_panel_off(struct platform_device *pdev)
{
	int ret;
	ret = mddi_queue_register_write(0x1000, 0, 0, 0);
	mddi_wait(5);
	ret = mddi_queue_register_write(0x2800, 0, 0, 0);
	return ret;
}

static void mddi_nt35582_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;

	bl_level = mfd->bl_level;

#ifdef CONFIG_PMIC8058_PWM
	if (bl_pwm) {
		pwm_config(bl_pwm, DUTY_LEVEL * bl_level, PWM_PERIOD);
		pwm_enable(bl_pwm);
	}
#endif
}

static int __devinit nt35582_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mddi_nt35582_pdata = pdev->dev.platform_data;
		return 0;
	}

#ifdef CONFIG_PMIC8058_PWM
	bl_pwm = pwm_request(mddi_nt35582_pdata->gpio, "backlight");
	if (bl_pwm == NULL || IS_ERR(bl_pwm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm = NULL;
	}

	/* Add default sensitivity to make sure screen won't turn off.*/
	if (bl_pwm) {
		pwm_config(bl_pwm, DUTY_LEVEL * 150, PWM_PERIOD);
		pwm_enable(bl_pwm);
	}

	printk(KERN_INFO "%s: bl_pwm=%x LPG_chan=%d\n",
		__func__, (int) bl_pwm, (int)mddi_nt35582_pdata->gpio);
#endif

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = nt35582_probe,
	.driver = {
		.name = "mddi_nt35582_wvga",
	},
};

static struct msm_fb_panel_data nt35582_panel_data = {
	.on = mddi_nt35582_panel_on,
	.off = mddi_nt35582_panel_off,
	.set_backlight = mddi_nt35582_panel_set_backlight,
};

static struct platform_device this_device = {
	.name = "mddi_nt35582_wvga",
	.id = 1,
	.dev = {
		.platform_data = &nt35582_panel_data,
	}
};

static int __init mddi_nt35582_wvga_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("mddi_nt35582_wvga"))
		return 0;
#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &nt35582_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = MDDI_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 16;
	pinfo->lcd.vsync_enable = TRUE;
	pinfo->lcd.refx100 = 4000;
	pinfo->mddi.is_type1 = TRUE;
	pinfo->lcd.v_back_porch = 0;
	pinfo->lcd.v_front_porch = 0;
	pinfo->lcd.v_pulse_width = 22;
	pinfo->lcd.hw_vsync_mode = FALSE;
	pinfo->lcd.vsync_notifier_period = (1 * HZ);
	pinfo->bl_max = BL_MAX;
	pinfo->bl_min = 1;
	pinfo->clk_rate = 192000000;
	pinfo->clk_min = 192000000;
	pinfo->clk_max = 192000000;
	pinfo->fb_num = 2;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			__func__);
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

module_init(mddi_nt35582_wvga_init);

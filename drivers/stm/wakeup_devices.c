/*
 * Copyright (C) 2010  STMicroelectronics
 * Author: Francesco M. Virlinzi  <francesco.virlinzi@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License V.2 ONLY.  See linux/COPYING for more information.
 */

#include <linux/stm/wakeup_devices.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/export.h>

static int wokenup_by;

int stm_get_wakeup_reason(void)
{
	return wokenup_by;
}

void stm_set_wakeup_reason(int irq)
{
	wokenup_by = irq;
}

static void stm_wake_init(struct stm_wakeup_devices *wkd)
{
	memset(wkd, 0, sizeof(*wkd));
}

static int __check_wakeup_device(struct device *dev, void *data)
{
	struct stm_wakeup_devices *wkd = (struct stm_wakeup_devices *)data;

	if (device_may_wakeup(dev)) {
		pr_info("[STM][PM] -> device %s can wakeup\n", dev_name(dev));
		if (!strcmp(dev_name(dev), "lirc-stm"))
			wkd->lirc_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "hdmi"))
			wkd->hdmi_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth.0"))
			wkd->stm_mac0_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stmmaceth.1"))
			wkd->stm_mac1_can_wakeup = 1;
		else if (!strcmp(dev_name(dev), "stm-hdmi-cec"))
			wkd->hdmi_cec = 1;
		else if (!strcmp(dev_name(dev), "stm-hdmi-hot"))
			wkd->hdmi_hotplug = 1;
		else if (!strcmp(dev_name(dev), "stm-kscan"))
			wkd->kscan = 1;
		else if (!strcmp(dev_name(dev), "stm-rtc"))
			wkd->rtc = 1;
		else if (!strcmp(dev_name(dev), "stm-asc"))
			wkd->asc = 1;
		else if (!strncmp(dev_name(dev), "stm-asc.", 8))
			wkd->asc = 1;
		else if (!strncmp(dev_name(dev), "stm-rtc-sbc", 11))
			wkd->rtc_sbc = 1;

	}
	return 0;
}

int stm_check_wakeup_devices(struct stm_wakeup_devices *wkd)
{
	stm_wake_init(wkd);
	bus_for_each_dev(&platform_bus_type, NULL, wkd, __check_wakeup_device);
	return 0;
}

EXPORT_SYMBOL(stm_check_wakeup_devices);

#ifdef CONFIG_STM_LPM

#include <linux/stm/lpm.h>

void stm_notify_wakeup_devices(void)
{
	struct stm_wakeup_devices wkd;
	unsigned int c_wk_device = 0;

	stm_check_wakeup_devices(&wkd);

	c_wk_device |= wkd.lirc_can_wakeup ? STM_LPM_WAKEUP_IR : 0;
	c_wk_device |= wkd.stm_mac0_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd.stm_mac1_can_wakeup ? STM_LPM_WAKEUP_WOL : 0;
	c_wk_device |= wkd.hdmi_cec ? STM_LPM_WAKEUP_CEC : 0;
	c_wk_device |= wkd.hdmi_hotplug ? STM_LPM_WAKEUP_HPD : 0;
	c_wk_device |= wkd.kscan ? STM_LPM_WAKEUP_FRP : 0;
	c_wk_device |= wkd.asc ? STM_LPM_WAKEUP_ASC : 0;
	c_wk_device |= wkd.rtc ? STM_LPM_WAKEUP_RTC : 0;
	c_wk_device |= wkd.rtc_sbc ? STM_LPM_WAKEUP_RTC : 0;

	stm_lpm_set_wakeup_device(c_wk_device | STM_LPM_WAKEUP_PIO);
}
#else
void stm_notify_wakeup_devices(void)
{
	return 0;
}
#endif

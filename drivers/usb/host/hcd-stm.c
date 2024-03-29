/*
 * HCD (Host Controller Driver) for USB.
 *
 * Copyright (c) 2009 STMicroelectronics Limited
 * Author: Francesco Virlinzi
 *
 * Bus Glue for STMicroelectronics STx710x devices.
 *
 * This file is licenced under the GPL.
 */

#include <linux/platform_device.h>
#include <linux/stm/platform.h>
#include <linux/stm/device.h>
#include <linux/stm/amba_bridge.h>
#include <linux/pm_runtime.h>
#include <linux/pm_clock.h>
#include <linux/delay.h>
#include <linux/usb.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include "hcd-stm.h"

#undef dgb_print

#ifdef CONFIG_USB_DEBUG
#define dgb_print(fmt, args...)			\
		pr_debug("%s: " fmt, __func__ , ## args)
#else
#define dgb_print(fmt, args...)
#endif

/*
 * USB-IP needs 3 clocks:
 * - a 48 MHz oscillator (to generate a final 480 MHz)
 * - a 100 MHz oscillator (for the NI)
 * - an oscillator for Phy
 * other clocks are generated internally using
 */

static int stm_usb_boot(struct platform_device *pdev)
{
	struct stm_plat_usb_data *pl_data = pdev->dev.platform_data;
	struct drv_usb_data *usb_data = platform_get_drvdata(pdev);
	void *wrapper_base = usb_data->ahb2stbus_wrapper_glue_base;
	unsigned long reg;

	if (pl_data->flags &
		(STM_PLAT_USB_FLAGS_STRAP_8BIT |
		 STM_PLAT_USB_FLAGS_STRAP_16BIT)) {
		/* Set strap mode */
		reg = readl(wrapper_base + AHB2STBUS_STRAP_OFFSET);
		if (pl_data->flags & STM_PLAT_USB_FLAGS_STRAP_16BIT)
			reg |= AHB2STBUS_STRAP_16_BIT;
		else
			reg &= ~AHB2STBUS_STRAP_16_BIT;
		writel(reg, wrapper_base + AHB2STBUS_STRAP_OFFSET);
	}

	if (pl_data->flags & STM_PLAT_USB_FLAGS_STRAP_PLL) {
		/* Start PLL */
		reg = readl(wrapper_base + AHB2STBUS_STRAP_OFFSET);
		writel(reg | AHB2STBUS_STRAP_PLL,
			wrapper_base + AHB2STBUS_STRAP_OFFSET);
		mdelay(30);
		writel(reg & (~AHB2STBUS_STRAP_PLL),
			wrapper_base + AHB2STBUS_STRAP_OFFSET);
		mdelay(30);
	}

	stm_amba_bridge_init(usb_data->amba_bridge);

	return 0;
}

static int stm_usb_remove(struct platform_device *pdev)
{
	struct drv_usb_data *dr_data = platform_get_drvdata(pdev);

	platform_device_unregister(dr_data->ehci_device);
	platform_device_unregister(dr_data->ohci_device);

	pm_clk_destroy(&pdev->dev);

	stm_device_power(dr_data->device_state, stm_device_power_off);

	return 0;
}

/*
 * Slightly modified version of platform_device_register_simple()
 * which assigns parent and has no resources.
 */
static struct platform_device
*stm_usb_device_create(const char *name, int id, struct platform_device *parent)
{
	struct platform_device *pdev;
	int retval;

	pdev = platform_device_alloc(name, id);
	if (!pdev) {
		retval = -ENOMEM;
		goto error;
	}

	pdev->dev.parent = &parent->dev;
	pdev->dev.dma_mask = parent->dev.dma_mask;
	pdev->dev.coherent_dma_mask = parent->dev.coherent_dma_mask;

	retval = platform_device_add(pdev);
	if (retval)
		goto error;

	return pdev;

error:
	platform_device_put(pdev);
	return ERR_PTR(retval);
}

static int __devinit stm_usb_probe(struct platform_device *pdev)
{
	struct stm_plat_usb_data *plat_data = pdev->dev.platform_data;
	struct drv_usb_data *dr_data;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i;
	static __devinitdata char *usb_clks_n[] = {
		"usb_48_clk",
		"usb_ic_clk",
		"usb_phy_clk"
	};
	resource_size_t len;
	void __iomem *amba_base;

	dgb_print("\n");
	dr_data = devm_kzalloc(dev, sizeof(*dr_data), GFP_KERNEL);
	if (!dr_data)
		return -ENOMEM;

	platform_set_drvdata(pdev, dr_data);

	pm_clk_init(dev);
	for (i = 0; i < ARRAY_SIZE(usb_clks_n); ++i) {
		struct clk *clk = clk_get(dev, usb_clks_n[i]);
		if (IS_ERR(clk)) {
			pr_warning("clk %s not found\n", usb_clks_n[i]);
			continue;
		}
		clk_enable(clk);
		if (!i)
			/*
			 * On some chip the USB_48_CLK comes from
			 * ClockGen_B. In this case a clk_set_rate
			 * is welcome because the code becomes
			 * target_pack independant
			 */
			clk_set_rate(clk, 48000000);
		pm_clk_add(dev, usb_clks_n[i]);
	}

	dr_data->device_state =
		devm_stm_device_init(&pdev->dev, plat_data->device_config);
	if (!dr_data->device_state)
		return -EBUSY;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wrapper");
	if (!res)
		return -ENXIO;

	len = resource_size(res);
	if (devm_request_mem_region(dev, res->start, len, pdev->name) < 0)
		return -EBUSY;

	dr_data->ahb2stbus_wrapper_glue_base =
		devm_ioremap_nocache(dev, res->start, len);

	if (!dr_data->ahb2stbus_wrapper_glue_base)
		return -EFAULT;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "protocol");
	if (!res)
		return -ENXIO;

	len = resource_size(res);
	if (devm_request_mem_region(dev, res->start, len, pdev->name) < 0)
		return -EBUSY;

	amba_base = devm_ioremap_nocache(dev, res->start, len);
	if (!amba_base)
		return -EFAULT;

	dr_data->amba_bridge =  stm_amba_bridge_create(dev, amba_base,
						       plat_data->amba_config);
	if (IS_ERR(dr_data->amba_bridge))
		return PTR_ERR(dr_data->amba_bridge);

	stm_usb_boot(pdev);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ehci");
	if (res) {
		dr_data->ehci_device = stm_usb_device_create("stm-ehci",
			pdev->id, pdev);
		if (IS_ERR(dr_data->ehci_device))
			return PTR_ERR(dr_data->ehci_device);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ohci");
	if (res) {
		dr_data->ohci_device =
			stm_usb_device_create("stm-ohci", pdev->id, pdev);
		if (IS_ERR(dr_data->ohci_device)) {
			platform_device_del(dr_data->ehci_device);
			return PTR_ERR(dr_data->ohci_device);
		}
	}

	/* Initialize the pm_runtime fields */
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	return 0;
}

static void stm_usb_shutdown(struct platform_device *pdev)
{
	struct drv_usb_data *dr_data = platform_get_drvdata(pdev);
	dgb_print("\n");

	pm_clk_destroy(&pdev->dev);
	stm_device_power(dr_data->device_state, stm_device_power_off);
}

#ifdef CONFIG_PM
static int stm_usb_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drv_usb_data *dr_data = dev_get_drvdata(dev);
	struct stm_plat_usb_data *pl_data = pdev->dev.platform_data;
	void *wrapper_base = dr_data->ahb2stbus_wrapper_glue_base;
	long reg;
	dgb_print("\n");

#ifdef CONFIG_PM_RUNTIME
	if (pm_runtime_status_suspended(dev))
		return 0; /* usb already suspended via runtime_suspend */
#endif

	if (pl_data->flags & STM_PLAT_USB_FLAGS_STRAP_PLL) {
		/* PLL turned off */
		reg = readl(wrapper_base + AHB2STBUS_STRAP_OFFSET);
		writel(reg | AHB2STBUS_STRAP_PLL,
			wrapper_base + AHB2STBUS_STRAP_OFFSET);
	}

	writel(0, wrapper_base + AHB2STBUS_STRAP_OFFSET);

	stm_device_power(dr_data->device_state, stm_device_power_off);

	return pm_clk_suspend(dev);
}

static int stm_usb_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drv_usb_data *dr_data = dev_get_drvdata(dev);
	int ret;

#ifdef CONFIG_PM_RUNTIME
	if (pm_runtime_status_suspended(dev))
		return 0; /* usb wants resume via runtime_resume... */
#endif

	dgb_print("\n");

	ret = pm_clk_resume(dev);
	if (ret)
		return ret;

	stm_device_setup(dr_data->device_state);

	stm_device_power(dr_data->device_state, stm_device_power_on);

	stm_usb_boot(pdev);

	return 0;
}
#else
#define stm_usb_suspend NULL
#define stm_usb_resume NULL
#endif

static struct dev_pm_ops stm_usb_pm = {
	.suspend = stm_usb_suspend,  /* on standby/memstandby */
	.resume = stm_usb_resume,    /* resume from standby/memstandby */

	.freeze = stm_usb_suspend,
	.restore = stm_usb_resume,

	.runtime_suspend = stm_usb_suspend,
	.runtime_resume = stm_usb_resume,
};

static struct platform_driver stm_usb_driver = {
	.driver = {
		.name = "stm-usb",
		.owner = THIS_MODULE,
		.pm = &stm_usb_pm,
	},
	.probe = stm_usb_probe,
	.shutdown = stm_usb_shutdown,
	.remove = stm_usb_remove,
};

static int __init stm_usb_init(void)
{
	return platform_driver_register(&stm_usb_driver);
}

static void __exit stm_usb_exit(void)
{
	platform_driver_unregister(&stm_usb_driver);
}

MODULE_LICENSE("GPL");

module_init(stm_usb_init);
module_exit(stm_usb_exit);

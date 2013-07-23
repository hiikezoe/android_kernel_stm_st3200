/*
 * (c) 2012 STMicroelectronics Limited
 * Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/stm/pad.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/soc.h>
#include <linux/stm/fli7610.h>

#ifdef CONFIG_ARM
#include <asm/mach/map.h>
#include <mach/soc-fli7610.h>
#include <mach/hardware.h>
#endif

#ifdef CONFIG_SUPERH
#include <asm/irq-ilc.h>
#endif

#include "pio-control.h"

/* Custom PAD configuration for the MMC Host controller */
#define FLI7610_PIO_MMC_CLK_OUT(_port, _pin) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_bidir_pull_up, \
		.function = 1, \
		.name = "MMCCLK", \
	}

#define FLI7610_PIO_MMC_OUT(_port, _pin) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_out, \
		.function = 1, \
	}
#define FLI7610_PIO_MMC_BIDIR(_port, _pin) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_bidir_pull_up, \
		.function = 1, \
	}
#define FLI7610_PIO_MMC_IN(_port, _pin) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_in, \
		.function = 1, \
	}

static struct stm_pad_config fli7610_mmc_pad_config = {
	.gpios_num = 14,
	.gpios = (struct stm_pad_gpio[]){
		 FLI7610_PIO_MMC_CLK_OUT(24, 1),
		 FLI7610_PIO_MMC_BIDIR(24, 2),	/* MMC command */
		 FLI7610_PIO_MMC_IN(24, 3),	/* MMC Card Detect */
		 FLI7610_PIO_MMC_BIDIR(24, 4),	/* MMC Data[0] */
		 FLI7610_PIO_MMC_BIDIR(24, 5),	/* MMC Data[1] */
		 FLI7610_PIO_MMC_BIDIR(24, 6),	/* MMC Data[2] */
		 FLI7610_PIO_MMC_BIDIR(24, 7),	/* MMC Data[3] */
		 FLI7610_PIO_MMC_BIDIR(25, 0),	/* MMC Data[4] */
		 FLI7610_PIO_MMC_BIDIR(25, 1),	/* MMC Data[5] */
		 FLI7610_PIO_MMC_BIDIR(25, 2),	/* MMC Data[6] */
		 FLI7610_PIO_MMC_BIDIR(25, 3),	/* MMC Data[7] */
		 FLI7610_PIO_MMC_IN(25, 4),	/* MMC Write Protection */
		 FLI7610_PIO_MMC_OUT(25, 6),	/* MMC Card PWR */
		 FLI7610_PIO_MMC_OUT(25, 7),	/* MMC LED on */
	 },
};

static struct stm_mmc_platform_data fli7610_mmc_platform_data = {
	.init = &mmc_claim_resource,
	.exit = &mmc_release_resource,
	.custom_cfg = &fli7610_mmc_pad_config,
	.nonremovable = false,
};

static struct platform_device fli7610_mmc_device = {
	.name = "sdhci-stm",
	.id = 0,
	.num_resources = 2,
	.resource = (struct resource[]){
		STM_PLAT_RESOURCE_MEM(0xfe81e000, 0x1000),
		FLI7610_RESOURCE_IRQ_NAMED("mmcirq", 145),
		},
	.dev = {
		.platform_data = &fli7610_mmc_platform_data,
		}
};

void __init fli7610_configure_mmc(int emmc)
{
	struct stm_mmc_platform_data *plat_data;
	struct sysconf_field *sc = sysconf_claim(TAE_SYSCONF(390), 1, 1,
						 "MMC_CONF_CARD_DET_POLARITY");
	/* Polarity of HSMMC_CARD_Detect is not inverted */
	sysconf_write(sc, 1);

	plat_data = &fli7610_mmc_platform_data;

	if (emmc)
		plat_data->nonremovable = true;

	platform_device_register(&fli7610_mmc_device);
}
/* MiPHY resources -------------------------------------------------------- */

#define MAX_PORTS    2
#define PCIE_BASE    0xfe800000
#define PCIE_UPORT_BASE    (PCIE_BASE + 0x4000)
#define PCIE_UPORT_REG_SIZE  (0xFF)

static enum miphy_mode fli7610_miphy_modes[MAX_PORTS];
static struct sysconf_field *sc_pcie_mp_select;


static void fli7610_pcie_mp_select(int port)
{
	BUG_ON(port < 0 || port > 1);
	sysconf_write(sc_pcie_mp_select, port);
}

struct stm_plat_pcie_mp_data fli7610_pcie_mp_platform_data = {
	.style_id = ID_MIPHY365X,
	.miphy_first = 0,
	.miphy_count = MAX_PORTS,
	.miphy_modes = fli7610_miphy_modes,
	.mp_select = fli7610_pcie_mp_select,
};
static struct platform_device fli7610_pcie_mp_device = {
	.name   = "pcie-mp",
	.id     = 0,
	.num_resources = 1,
	.resource = (struct resource[]) {
		[0] = {
			.start = PCIE_UPORT_BASE,
			.end   = PCIE_UPORT_BASE + PCIE_UPORT_REG_SIZE,
			.flags = IORESOURCE_MEM,
		},
	},
	.dev = {
		.platform_data = &fli7610_pcie_mp_platform_data,
	}
};

static int __init fli7610_configure_miphy_uport(void)
{
	sc_pcie_mp_select = sysconf_claim(TAE_SYSCONF(393), 0, 0, "pcie-mp");
	BUG_ON(!sc_pcie_mp_select);

	return platform_device_register(&fli7610_pcie_mp_device);
}

void __init fli7610_configure_miphy(struct fli7610_miphy_config *config)
{
	memcpy(fli7610_miphy_modes, config->modes, sizeof(fli7610_miphy_modes));

	fli7610_configure_miphy_uport();
}

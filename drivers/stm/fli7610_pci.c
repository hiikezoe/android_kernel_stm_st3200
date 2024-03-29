/* STMicroelectronics Limited
 *
 * Author: Pawel Moll <pawel.moll@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/stm/pad.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/emi.h>
#include <linux/stm/fli7610.h>

#define PCIE_SOFT_RST_N_PCIE (1<<6) /* Active low !! */
#define PCIE_SYS_INT   (1<<5)
#define PCIE_P1_SSC_EN   (1<<4)
#define PCIE_APP_REQ_RETRY_EN  (1<<3)
#define PCIE_APP_LTSSM_ENABLE  (1<<2)
#define PCIE_APP_INIT_RST  (1<<1)
#define PCIE_DEVICE_TYPE (1<<0)

#define PCIE_DEFAULT_VAL (PCIE_SOFT_RST_N_PCIE | PCIE_DEVICE_TYPE)

static void fli7610_pcie_init(void *handle)
{
	struct sysconf_field *sc = (struct sysconf_field *) handle;

	/* Drive RST_N low, set device type */
	sysconf_write(sc, PCIE_DEVICE_TYPE);

	sysconf_write(sc, PCIE_DEFAULT_VAL);

	mdelay(1);
}

static void fli7610_pcie_enable_ltssm(void *handle)
{
	struct sysconf_field *sc = (struct sysconf_field *) handle;

	sysconf_write(sc, PCIE_DEFAULT_VAL | PCIE_APP_LTSSM_ENABLE);
}

static void fli7610_pcie_disable_ltssm(void *handle)
{
	struct sysconf_field *sc = (struct sysconf_field *) handle;

	sysconf_write(sc, PCIE_DEFAULT_VAL);
}

/* Ops to drive the platform specific bits of the interface */
static struct stm_plat_pcie_ops fli7610_pcie_ops = {
	.init          = fli7610_pcie_init,
	.enable_ltssm  = fli7610_pcie_enable_ltssm,
	.disable_ltssm = fli7610_pcie_disable_ltssm,
};

/* PCI express support */
#define PCIE_MEM_START 0x20000000
#define PCIE_MEM_SIZE  0x20000000
#define PCIE_CONFIG_SIZE (64*1024)

#define LMI_START 0x40000000
#define LMI_SIZE  0x80000000

static struct stm_plat_pcie_config fli7610_plat_pcie_config = {
	.ahb_val = 0x264207,
	.ops = &fli7610_pcie_ops,
	.pcie_window.start = PCIE_MEM_START,
	.pcie_window.size = PCIE_MEM_SIZE - PCIE_CONFIG_SIZE,
	.pcie_window.lmi_start = LMI_START,
	.pcie_window.lmi_size = LMI_SIZE,
};

#define MSI_FIRST_IRQ  (NR_IRQS - 32)
#define MSI_LAST_IRQ (NR_IRQS - 1)

static struct platform_device fli7610_pcie_device = {
	.name = "pcie_stm",
	.id = -1,
	.num_resources = 10,
	.resource = (struct resource[]) {
		/* Place 64K Config window at end of memory block */
		STM_PLAT_RESOURCE_MEM_NAMED("pcie config",
			PCIE_MEM_START + PCIE_MEM_SIZE - PCIE_CONFIG_SIZE,
			PCIE_CONFIG_SIZE),
		STM_PLAT_RESOURCE_MEM_NAMED("pcie cntrl", 0xfe800000, 0x1000),
		STM_PLAT_RESOURCE_MEM_NAMED("pcie ahb", 0xfe808000, 0x8),
		FLI7610_RESOURCE_IRQ_NAMED("pcie inta", 166),
		FLI7610_RESOURCE_IRQ_NAMED("pcie pme", 168),
		FLI7610_RESOURCE_IRQ_NAMED("pcie syserr", 169),
		FLI7610_RESOURCE_IRQ_NAMED("pcie link_rst", 170),
		FLI7610_RESOURCE_IRQ_NAMED("pcie cpl_timeout", 171),
		FLI7610_RESOURCE_IRQ_NAMED("msi mux", 167),
		{
			.start = MSI_FIRST_IRQ,
			.end  = MSI_LAST_IRQ,
			.name = "msi range",
			.flags = IORESOURCE_IRQ,
		}
	},
	.dev.platform_data = &fli7610_plat_pcie_config,
};


void __init fli7610_configure_pcie(struct fli7610_pcie_config *config)
{
	struct sysconf_field *sc;

	sc = sysconf_claim(TAE_SYSCONF(392), 0, 6, "pcie");

	BUG_ON(!sc);

	fli7610_plat_pcie_config.ops_handle = sc;
	fli7610_plat_pcie_config.reset_gpio = config->reset_gpio;
	fli7610_plat_pcie_config.reset = config->reset;
	/* There is only one PCIe controller on Newman sharing
	 * the miphy0 with SATA Controller
	 */
	fli7610_plat_pcie_config.miphy_num = 0;

	platform_device_register(&fli7610_pcie_device);
}


/*
 * arch/arm/mach-stm/soc-fli7610.c
 *
 * Copyright (C) 2011 STMicroelectronics Limited.
 * Author: Srinivas Kandagatla <srinivas.kandagatla@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/stm/clk.h>
#include <linux/stm/fli7610-periphs.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/smp_twd.h>
#include <asm/smp_gt.h>
#include <asm/hardware/gic.h>

#include <linux/stm/abap.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/soc-fli7610.h>
#include <mach/mpe41.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

#include "core.h"

static struct map_desc fli7610_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(MPE41_SCU_BASE),
		.pfn		= __phys_to_pfn(MPE41_SCU_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_GIC_DIST_BASE),
		.pfn		= __phys_to_pfn(MPE41_GIC_DIST_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual        = IO_ADDRESS(MPE41_PL310_BASE),
		.pfn            = __phys_to_pfn(MPE41_PL310_BASE),
		.length         = SZ_16K,
		.type           = MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_TAE_ASC0_BASE),
		.pfn		= __phys_to_pfn(FLI7610_TAE_ASC0_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_PIO_TAE_SBC_BASE),
		.pfn		= __phys_to_pfn(FLI7610_PIO_TAE_SBC_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_PIO_1_TAE_BASE),
		.pfn		= __phys_to_pfn(FLI7610_PIO_1_TAE_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_PIO_2_TAE_BASE),
		.pfn		= __phys_to_pfn(FLI7610_PIO_2_TAE_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,

	}, {
		.virtual	= IO_ADDRESS(FLI7610_PIO_3_TAE_BASE),
		.pfn		= __phys_to_pfn(FLI7610_PIO_3_TAE_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_PIO_RIGHT_BASE),
		.pfn		= __phys_to_pfn(MPE41_PIO_RIGHT_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_PIO_LEFT_BASE),
		.pfn		= __phys_to_pfn(MPE41_PIO_LEFT_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_SBC_SYSCONF_BASE),
		.pfn		= __phys_to_pfn(FLI7610_SBC_SYSCONF_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_TAE_SYSCONF_BANK1_BASE),
		.pfn		= __phys_to_pfn(FLI7610_TAE_SYSCONF_BANK1_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_TAE_SYSCONF_BANK2_BASE),
		.pfn		= __phys_to_pfn(FLI7610_TAE_SYSCONF_BANK2_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,

	}, {
		.virtual	= IO_ADDRESS(FLI7610_TAE_SYSCONF_BANK3_BASE),
		.pfn		= __phys_to_pfn(FLI7610_TAE_SYSCONF_BANK3_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,

	}, {
		.virtual	= IO_ADDRESS(FLI7610_TAE_SYSCONF_BANK4_BASE),
		.pfn		= __phys_to_pfn(FLI7610_TAE_SYSCONF_BANK4_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_LEFT_SYSCONF_BASE),
		.pfn		= __phys_to_pfn(MPE41_LEFT_SYSCONF_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_RIGHT_SYSCONF_BASE),
		.pfn		= __phys_to_pfn(MPE41_RIGHT_SYSCONF_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(MPE41_SYSTEM_SYSCONF_BASE),
		.pfn		= __phys_to_pfn(MPE41_SYSTEM_SYSCONF_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_SBC_LPM_CONF_BASE),
		.pfn		= __phys_to_pfn(FLI7610_SBC_LPM_CONF_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(FLI7610_SBC_ASC0_BASE),
		.pfn		= __phys_to_pfn(FLI7610_SBC_ASC0_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_ADDRESS(ABAP_PRG_BASE),
		.pfn		= __phys_to_pfn(ABAP_PRG_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}
};

void __init fli7610_map_io(void)
{
	iotable_init(fli7610_io_desc, ARRAY_SIZE(fli7610_io_desc));
}


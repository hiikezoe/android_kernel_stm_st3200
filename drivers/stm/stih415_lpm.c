/*---------------------------------------------------------------------------
* /drivers/stm/stxh415_lpm.c
* Copyright (C) 2011 STMicroelectronics Limited
* Author:Francesco Virlinzi <francesco.virlinzi@st.com>
* Author:Pooja Agrawal <pooja.agrawal@st.com>
* Author:Udit Kumar <udit-dlh.kumar@st.cm>
* May be copied or modified under the terms of the GNU General Public
* License.  See linux/COPYING for more information.
*----------------------------------------------------------------------------
*/

#include <linux/platform_device.h>
#include <linux/stm/soc.h>
#include <linux/stm/platform.h>
#include <linux/stm/stih415.h>

static struct platform_device stm_lpm_device = {
	.name = "stm-lpm",
	.id = -1,
	.num_resources = 4,
	.resource = (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0xfe400000, 0xa0000),
		/* mailbox */
		STM_PLAT_RESOURCE_MEM(0xfe4b4000, 0x400),
		/* configuration register */
		STM_PLAT_RESOURCE_MEM(0xfe4b5100, 0x200),
		STIH415_RESOURCE_IRQ(215),
	},
};

const char *lpm_get_cpu_type(void)
{
	return stm_soc();
}

static int __init stxh415_add_lpm(void)
{
	return platform_device_register(&stm_lpm_device);
}

module_init(stxh415_add_lpm);

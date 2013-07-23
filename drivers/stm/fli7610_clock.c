/*
 * Copyright (C) 2011 STMicroelectronics Limited
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * Code to handle the clock aliases on the Fli7610.
 */

#include <linux/init.h>
#include <linux/stm/clk.h>

int __init plat_clk_alias_init(void)
{
	clk_add_alias("cpu_clk", NULL, "CLKM_A9", NULL);
	clk_add_alias("gpu_clk", NULL, "CLKM_GPU", NULL);

	/*
	 * arm_periph_clk does not exist on SH4 on which case this will
	 * fail silently. Alternatively the local/global timer
	 * may not actually be used, but there is no harm in creating
	 * the aliases anyway.
	 */
	clk_add_alias(NULL, "smp_twd", "arm_periph_clk", NULL);
	clk_add_alias(NULL, "smp_gt", "arm_periph_clk", NULL);

	/* comms clk */
	clk_add_alias("comms_clk", NULL, "CLKA_IC_100", NULL);
	/* module clk ?!?!?! */
	clk_add_alias("module_clk", NULL, "CLKA_IC_100", NULL);
	/* EMI clk */
	clk_add_alias("emi_clk", NULL, "CLKA_EMI", NULL);
	/* SBC clk */
	clk_add_alias("sbc_comms_clk", NULL, "CLK_SYSIN", NULL);

	/* fdmas MPE41 clocks */
	clk_add_alias("fdma_slim_clk", "stm-fdma.0", "CLKM_FDMA_10", NULL);
	clk_add_alias("fdma_slim_clk", "stm-fdma.1", "CLKM_FDMA_11", NULL);
	clk_add_alias("fdma_slim_clk", "stm-fdma.2", "CLKM_FDMA_12", NULL);
	clk_add_alias("fdma_hi_clk", "stm-fdma.0", "CLKM_ICN_TS", NULL);
	clk_add_alias("fdma_hi_clk", "stm-fdma.1", "CLKM_ICN_TS", NULL);
	clk_add_alias("fdma_hi_clk", "stm-fdma.2", "CLKM_ICN_TS", NULL);
	clk_add_alias("fdma_low_clk", "stm-fdma.0", "CLKM_ICN_REG_LP_10", NULL);
	clk_add_alias("fdma_low_clk", "stm-fdma.1", "CLKM_ICN_REG_LP_10", NULL);
	clk_add_alias("fdma_low_clk", "stm-fdma.2", "CLKM_ICN_REG_LP_10", NULL);
	clk_add_alias("fdma_ic_clk", "stm-fdma.0", "CLKM_ICN_TS", NULL);
	clk_add_alias("fdma_ic_clk", "stm-fdma.1", "CLKM_ICN_TS", NULL);
	clk_add_alias("fdma_ic_clk", "stm-fdma.2", "CLKM_ICN_TS", NULL);

	/* fdmas TAE clocks */
	clk_add_alias("fdma_slim_clk", "stm-fdma.3", "CLKA_FDMA", NULL);
	clk_add_alias("fdma_slim_clk", "stm-fdma.4", "CLKA_FDMA", NULL);
	clk_add_alias("fdma_hi_clk", "stm-fdma.3", "CLKA_IC_200",  NULL);
	clk_add_alias("fdma_hi_clk", "stm-fdma.4", "CLKA_IC_200",  NULL);
	clk_add_alias("fdma_low_clk", "stm-fdma.3", "CLKA_IC_100", NULL);
	clk_add_alias("fdma_low_clk", "stm-fdma.4", "CLKA_IC_100", NULL);
	clk_add_alias("fdma_ic_clk", "stm-fdma.3", "CLKA_IC_200", NULL);
	clk_add_alias("fdma_ic_clk", "stm-fdma.4", "CLKA_IC_200", NULL);

	clk_add_alias("usb_48_clk", "stm-usb.0", "CLK_USB1_48", NULL);
	clk_add_alias("usb_48_clk", "stm-usb.1", "CLK_USB2_48", NULL);
	clk_add_alias("usb_48_clk", "stm-usb.2", "CLK_USB1_60", NULL);

	clk_add_alias("usb_phy_clk", NULL, "USB2_TRIPLE_PHY", NULL);
	clk_add_alias("usb_ic_clk", NULL, "CLKA_IC_200", NULL);

	/* Uniperipheral players all use the same clock */
	clk_add_alias("uni_player_clk", NULL, "CLK_256FS_FREE_RUN", NULL);

	/* SPDIF RX clock */
	clk_add_alias("spdif_rx_clk", NULL, "CLK_SPDIF_RX", NULL);

	/* SDHCI clocks */
	clk_add_alias(NULL, "sdhci-stm.0", "CLK_SD_MS",  NULL);

	/* MAC clock */
	clk_add_alias("stmmaceth", NULL, "CLKA_GMAC", NULL);

	return 0;
}

/*
 * arch/arm/mach-stm/board-b2000.c
 *
 * Copyright (C) 2011 STMicroelectronics Limited.
 * Author: Stuart Menefy <stuart.menefy@st.com>
 * 	   Srinivas Kandagatla <srinivas.kandagatla@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mdio-gpio.h>
#include <linux/input.h>
#include <linux/phy.h>
#include <linux/leds.h>
#include <linux/stm/platform.h>
#include <linux/stm/stih415.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/memory.h>
#include <asm/delay.h>

#include <mach/soc-stih415.h>
#include <mach/mpe41.h>
#include <mach/hardware.h>

#include <linux/stm/pad.h>
#include <linux/stm/pio-control.h>

static struct stm_pad_config b20002a_tsin_pad_config = {
	.gpios_num = 30,
	.gpios = (struct stm_pad_gpio []) {
		STM_PAD_PIO_IN_NAMED_RETIME(5, 4, 1, "tsin0serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 3, 1, "tsin0btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 1, 1, "tsin0valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 0, 1, "tsin0error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 2, 1, "tsin0pkclk", RET_SE_NICLK_IO(0, 0)),

		STM_PAD_PIO_IN_NAMED_RETIME(7, 0, 1, "tsin1serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(6, 7, 1, "tsin1btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(6, 5, 1, "tsin1valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(6, 4, 1, "tsin1error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(6, 6, 1, "tsin1pkclk", RET_SE_NICLK_IO(0, 0)),

		STM_PAD_PIO_IN_NAMED_RETIME(8, 4, 1, "tsin2serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 3, 1, "tsin2btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 1, 1, "tsin2valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 0, 1, "tsin2error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 2, 1, "tsin2pkclk", RET_SE_NICLK_IO(0, 0)),

		STM_PAD_PIO_IN_NAMED_RETIME(9, 1, 1, "tsin3serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(9, 0, 1, "tsin3btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 6, 1, "tsin3valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 5, 1, "tsin3error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(8, 7, 1, "tsin3pkclk", RET_SE_NICLK_IO(0, 0)),

		STM_PAD_PIO_IN_NAMED_RETIME(6, 1, 2, "tsin4serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(6, 0, 2, "tsin4btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 6, 2, "tsin4valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 5, 2, "tsin4error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(5, 7, 2, "tsin4pkclk", RET_SE_NICLK_IO(0, 0)),

		STM_PAD_PIO_IN_NAMED_RETIME(7, 5, 2, "tsin5serdata", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(7, 4, 2, "tsin5btclkin", RET_NICLK(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(7, 2, 2, "tsin5valid", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(7, 1, 2, "tsin5error", RET_SE_NICLK_IO(0, 0)),
		STM_PAD_PIO_IN_NAMED_RETIME(7, 3, 2, "tsin5pkclk", RET_SE_NICLK_IO(0, 0)),
	},
};

static void b2000_configure_tsin(bool freeze)
{
	static struct stm_pad_state *tsin_pad = NULL;

	if (!freeze) {
		tsin_pad = stm_pad_claim(&b20002a_tsin_pad_config, "tsin");
	}
	else {
		if (tsin_pad)
			stm_pad_release(tsin_pad);
		tsin_pad = NULL;
	}
}

static void __init b2000_init_early(void)
{
	printk("STMicroelectronics STiH415 (Orly) MBoard initialisation\n");

	stih415_early_device_init();

#if !defined(CONFIG_MACH_STM_B2000_B2011A_AUDIO)
	stih415_configure_asc(2, &(struct stih415_asc_config) {
			.hw_flow_control = 0,
			.is_console = 1 });
#endif
}

#define GMII0_PHY_NOT_RESET stm_gpio(25 /*106 -100 + 18 */, 2)
#define GMII1_PHY_NOT_RESET stm_gpio(4, 7)
#define GMII1_PHY_CLKOUT_NOT_TXCLK_SEL stm_gpio(2, 5)
#define HDMI_HOTPLUG	GMII1_PHY_CLKOUT_NOT_TXCLK_SEL
#define GMII0_PHY_CLKOUT_NOT_TXCLK_SEL stm_gpio(13, 4)

#if defined(CONFIG_MACH_STM_B2000_CN23_NONE)
static struct stm_pad_config stih415_hdmi_hp_pad_config = {
        .gpios_num = 1,
        .gpios = (struct stm_pad_gpio []) {
                STM_PAD_PIO_IN(2, 5, 1),      /* HDMI Hotplug */
        },
};
#endif

/* NAND Flash */
static struct stm_nand_bank_data b2000_nand_flash = {
	.csn            = 0,
	.options        = NAND_NO_AUTOINCR,
	.bbt_options	= NAND_BBT_USE_FLASH,
	.nr_partitions  = 2,
	.partitions     = (struct mtd_partition []) {
		{
			.name   = "NAND Flash 1",
			.offset = 0,
			.size   = 0x00800000
		}, {
			.name   = "NAND Flash 2",
			.offset = MTDPART_OFS_NXTBLK,
			.size   = MTDPART_SIZ_FULL
		},
	},
	.timing_data = &(struct stm_nand_timing_data) {
		.sig_setup      = 10,		/* times in ns */
		.sig_hold       = 10,
		.CE_deassert    = 0,
		.WE_to_RBn      = 100,
		.wr_on          = 10,
		.wr_off         = 30,
		.rd_on          = 10,
		.rd_off         = 30,
		.chip_delay     = 30,		/* in us */
	},
};

/* Serial FLASH */
static struct stm_plat_spifsm_data b2000_serial_flash =  {
	.name		= "n25q128",
	.nr_parts	= 2,
	.parts = (struct mtd_partition []) {
		{
			.name = "Serial Flash 1",
			.size = 0x00200000,
			.offset = 0,
		}, {
			.name = "Serial Flash 2",
			.size = MTDPART_SIZ_FULL,
			.offset = MTDPART_OFS_NXTBLK,
		},
	},
	.capabilities = {
		/* Capabilities may be overriden by SoC configuration */
		.dual_mode = 1,
		.quad_mode = 1,
	},
};

#if defined(CONFIG_MACH_STM_B2000_CN22_B2032_GIGA_MODE)
static void b2000_gmac0_txclk_select(int txclk_125_not_25_mhz)
{
	/* When 1000 speed is negotiated we have to set the PIO13[4]. */
	if (txclk_125_not_25_mhz)
		gpio_set_value(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL, 1);
	else
		gpio_set_value(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL, 0);
}
#endif

#if defined(CONFIG_MACH_STM_B2000_CN23_B2032_GIGA_MODE)
/*
 * On B2000B board PIO 2-5 conflicts with the HDMI hot-plug detection pin.
 * As we have a seperate 125clk pin in to the MAC, we might not need
 * tx clk selection.
 * Removing R59 from B2032A will solve the problem and TXCLK is
 * always connected to PHY TXCLK.
 * NOTE: This will be an issue if the B2032 is used with a 7108 as
 * 125clk comes from phy txclk, and so this signal is needed.
 */
/* GMII and RGMII on GMAC1 NOT Yet Supported
 * Noticed Packet drops with GMII &
 * RGMII does not work even for validation.
 */
static void b2000_gmac1_txclk_select(int txclk_125_not_25_mhz)
{
	/* When 1000 speed is negotiated we have to set the PIO2[5]. */
	if (txclk_125_not_25_mhz)
		gpio_set_value(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL, 1);
	else
		gpio_set_value(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL, 0);
}
#endif

#if defined(CONFIG_MACH_STM_B2000_CN22_B2035) || defined(CONFIG_MACH_STM_B2000_CN22_B2032)
static int b2000_gmii0_reset(struct mii_bus *bus)
{
	gpio_set_value(GMII0_PHY_NOT_RESET, 1);
	gpio_set_value(GMII0_PHY_NOT_RESET, 0);
	mdelay(10); /* 10 miliseconds is enough for everyone ;-) */
	gpio_set_value(GMII0_PHY_NOT_RESET, 1);
	mdelay(10); /* 10 miliseconds is enough for everyone ;-) */

	return 1;
}

/*
 * On GMAC0, it is noticed that some mdio register reads return value with
 * bit 15 set, however the waveforms on board show that they are
 * transmitted correctly. Which is a BUG at SOC level.
 * Some SOC's behave different to others w.r.t which registers are returned
 *  with 15 bit set
 *
 * Using mdio_gpio driver seems to solve the problem.
 */
#define STMMAC0_MDIO_GPIO_BUS	3
static struct platform_device stmmac0_mdio_gpio_bus = {
	.name = "mdio-gpio",
	.id = STMMAC0_MDIO_GPIO_BUS,
	.dev.platform_data = &(struct mdio_gpio_platform_data) {
		/* GMAC0 SMI */
		.mdc = stm_gpio(15, 5),
		.mdio = stm_gpio(15, 4),
		.reset = b2000_gmii0_reset,
	},
};
#endif

#if defined(CONFIG_MACH_STM_B2000_CN23_B2035) || defined(CONFIG_MACH_STM_B2000_CN23_B2032)

#if defined(CONFIG_MACH_STM_B2000_CN23_B2032_RGMII_MODE)
#error "RGMII mode on GMAC1 is not functional"
#endif
static int b2000_gmii1_reset(void *bus)
{
	gpio_set_value(GMII1_PHY_NOT_RESET, 1);
	gpio_set_value(GMII1_PHY_NOT_RESET, 0);
	mdelay(10); /* 10 miliseconds is enough for everyone ;-) */
	gpio_set_value(GMII1_PHY_NOT_RESET, 1);
	mdelay(10); /* 10 miliseconds is enough for everyone ;-) */

	return 1;
}

static struct stmmac_mdio_bus_data stmmac1_mdio_bus = {
	/* GMII connector CN23 */
	.phy_reset = &b2000_gmii1_reset,
	.phy_mask = 0,
};
#endif

static struct platform_device b2000_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev.platform_data = &(struct gpio_led_platform_data) {
		.num_leds = 1,
		.leds = (struct gpio_led[]) {
			{
				.name = "FP_LED",
				.default_trigger = "heartbeat",
				.gpio = stm_gpio(STIH415_PIO(105), 7),
			}
		},
	},
};

static struct platform_device *b2000_devices[] __initdata = {
	&b2000_leds,
};


static void b2000_ethphy_gpio_init(int cold_boot)
{
	/* Reset */
	if (cold_boot) {
		gpio_request(GMII0_PHY_NOT_RESET, "GMII0_PHY_NOT_RESET");
		gpio_request(GMII1_PHY_NOT_RESET, "GMII1_PHY_NOT_RESET");
	}
	gpio_direction_output(GMII0_PHY_NOT_RESET, 0);
	gpio_direction_output(GMII1_PHY_NOT_RESET, 0);

#if !defined(CONFIG_MACH_STM_B2000_CN23_NONE)
	/* Default to 100 Mbps */
	gpio_request(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL, "GMII1_TXCLK_SEL");
#if defined(CONFIG_MACH_STM_B2000_CN23_B2032_GIGA_MODE)
	gpio_direction_output(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL, 1);
#else
	gpio_direction_output(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL, 0);
#endif
	gpio_free(GMII1_PHY_CLKOUT_NOT_TXCLK_SEL);
#endif

#if !defined(CONFIG_MACH_STM_B2000_CN22_NONE)
	/* Default to 100 Mbps */
	gpio_request(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL, "GMII0_TXCLK_SEL");
	/* Can be ignored for RGMII on this PHY */
#if defined(CONFIG_MACH_STM_B2000_CN22_B2032_GIGA_MODE)
	gpio_direction_output(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL, 1);
#else
	gpio_direction_output(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL, 0);
#endif
	gpio_free(GMII0_PHY_CLKOUT_NOT_TXCLK_SEL);
#endif


#if defined(CONFIG_MACH_STM_B2000_CN22_B2035) || defined(CONFIG_MACH_STM_B2000_CN22_B2032)
	b2000_gmii0_reset(NULL);
#endif

#if defined(CONFIG_MACH_STM_B2000_CN23_B2035) || defined(CONFIG_MACH_STM_B2000_CN23_B2032)
	b2000_gmii1_reset(NULL);
#endif
}

static struct platform_device b2000_sbc_config = {
	.name = "stm-sbc",
	.dev.platform_data = &(struct stm_sbc_platform_data) {
		/* J31 should be not fitted, and J33 set to 1-2 */
		.gpio = stm_gpio(4, 3),
		.trigger_level = 1,
	},
};

static void __init b2000_init(void)
{
	stih415_configure_sbc(&b2000_sbc_config);

	b2000_ethphy_gpio_init(1);
#if defined(CONFIG_MACH_STM_B2000_CN23_NONE)
	/* Default to HDMI HotPlug */
	if (stm_pad_claim(&stih415_hdmi_hp_pad_config, "HDMI_Hotplug") == NULL)
		printk(KERN_ERR "Failed to claim HDMI-Hotplug pad!\n");
#endif
/*
 * B2032A (MII or GMII) Ethernet card
 * GMII Mode on B2032A needs R26 to be fitted with 51R
 * On B2000B board, to get GMAC0 working make sure that jumper
 * on PIN 9-10 on CN35 and CN36 are removed.
 *
 * For RGMII: Remove the J1 Jumper.
 * 100/10 Link: TXCLK_SELECTION can be ignored for this PHY.
 */

/*
 * B2035A (RMII + MMC(on CN22)) Ethernet + MMC card
 * B2035A board has IP101ALF PHY connected in RMII mode
 * and an MMC card
 * It is designed to be conneted to GMAC0 (CN22) to get MMC working,
 * however we can connect it to GMAC1 for RMII testing.
 */

/* GMAC0 */
#if defined(CONFIG_MACH_STM_B2000_CN22_B2035) || defined(CONFIG_MACH_STM_B2000_CN22_B2032)
	stih415_configure_ethernet(0, &(struct stih415_ethernet_config) {
#ifdef CONFIG_MACH_STM_B2000_CN22_B2035
			.interface = PHY_INTERFACE_MODE_RMII,
			.ext_clk = 0,
			.phy_addr = 9,
#endif /* CONFIG_MACH_STM_B2000_CN22_B2035 */

#ifdef CONFIG_MACH_STM_B2000_CN22_B2032
/* B2032 modified to support GMII */
#if defined(CONFIG_MACH_STM_B2000_CN22_B2032_GIGA_MODE)
			.txclk_select = b2000_gmac0_txclk_select,
#ifdef CONFIG_MACH_STM_B2000_CN22_B2032_GMII_MODE
			.interface = PHY_INTERFACE_MODE_GMII,
#else
			.interface = PHY_INTERFACE_MODE_RGMII_ID,
#endif /* CONFIG_MACH_STM_B2000_CN22_B2032_GMII_MODE */
#else
			.interface = PHY_INTERFACE_MODE_MII,
#endif /* CONFIG_MACH_STM_B2000_CN22_B2032_GIGA_MODE */
			.ext_clk = 1,
			.phy_addr = 1,
#endif /* CONFIG_MACH_STM_B2000_CN22_B2032 */

			.phy_bus_name = "gpio",
			.phy_bus = STMMAC0_MDIO_GPIO_BUS,});

#endif

/* GMAC1 */
#if !defined(CONFIG_MACH_STM_B2000_CN23_NONE)
	stih415_configure_ethernet(1, &(struct stih415_ethernet_config) {
#ifdef CONFIG_MACH_STM_B2000_CN23_B2035
			.interface = PHY_INTERFACE_MODE_RMII,
			.ext_clk = 0,
			.phy_addr = 9,
#endif /* CONFIG_MACH_STM_B2000_CN23_B2035 */

/* RGMII on GMAC1 has problems with TX side
 */
#ifdef CONFIG_MACH_STM_B2000_CN23_B2032
#if defined(CONFIG_MACH_STM_B2000_CN23_B2032_GIGA_MODE)
			.txclk_select = b2000_gmac1_txclk_select,
#ifdef CONFIG_MACH_STM_B2000_CN23_B2032_GMII_MODE
			.interface = PHY_INTERFACE_MODE_GMII,
#else
			.interface = PHY_INTERFACE_MODE_RGMII_ID,
#endif /* CONFIG_MACH_STM_B2000_CN23_B2032_GMII_MODE */
#else
			.interface = PHY_INTERFACE_MODE_MII,
#endif /* CONFIG_MACH_STM_B2000_CN23_B2032 */
			.ext_clk = 1,
			.phy_addr = 1,
#endif /* CONFIG_MACH_STM_B2000_CN23_B2032 */

			.phy_bus = 1,
			.mdio_bus_data = &stmmac1_mdio_bus,});
#endif /* CONFIG_MACH_STM_B2000_CN23_NONE */

	stih415_configure_usb(0);
	stih415_configure_usb(1);
	stih415_configure_usb(2);

	/* HDMI */
	stih415_configure_ssc_i2c(STIH415_SSC(1),
			&(struct stih415_ssc_config) {.i2c_speed = 100,});

#if !defined(CONFIG_MACH_STM_B2000_B2011A_AUDIO)
	/* Frontend I2C, make sure J17-J20 are configured accordingly */
	stih415_configure_ssc_i2c(STIH415_SSC(3),
			&(struct stih415_ssc_config) {.i2c_speed = 100,});
#endif

	/* Backend I2C, make sure J50, J51 are configured accordingly */
	stih415_configure_ssc_i2c(STIH415_SBC_SSC(0),
			&(struct stih415_ssc_config) {.i2c_speed = 100,});

	/* IR_IN */
	stih415_configure_lirc(&(struct stih415_lirc_config) {
			.rx_mode = stih415_lirc_rx_mode_ir, });

	stih415_configure_pwm(&(struct stih415_pwm_config) {
			.pwm = stih415_sbc_pwm,
			.out0_enabled = 1, });

#if defined(CONFIG_MACH_STM_B2000_CN22_B2035) || \
	defined(CONFIG_MACH_STM_B2000_CN22_B2048)
#ifdef CONFIG_MACH_STM_B2000_B2048_EMMC
	/* eMMC on board */
	stih415_configure_mmc(1);
#else
	/*
	 * In case of B2035 or B2048A without eMMC, we actually have the
	 * MMC/SD slot on the daughter board.
	 */
	stih415_configure_mmc(0);
#endif
#endif

	stih415_configure_nand(&(struct stm_nand_config) {
			.driver = stm_nand_afm,
			.nr_banks = 1,
			.banks = &b2000_nand_flash,
			.rbn.flex_connected = 1,});

	stih415_configure_spifsm(&b2000_serial_flash);

	stih415_configure_audio(&(struct stih415_audio_config) {
			.uni_player_3_spdif_enabled = 1,
			.uni_reader_0_spdif_enabled =
				IS_ENABLED(CONFIG_MACH_STM_B2000_B2011A_AUDIO),
		});

	b2000_configure_tsin(0);

	stih415_configure_keyscan(&(struct stm_keyscan_config) {
			.num_out_pads = 4,
			.num_in_pads = 4,
			.debounce_us = 5000,
			.keycodes = {
				/* in0 ,   in1  ,   in2 ,  in3  */
				KEY_F13, KEY_F9,  KEY_F5, KEY_F1,  /* out0 */
				KEY_F14, KEY_F10, KEY_F6, KEY_F2,  /* out1 */
				KEY_F15, KEY_F11, KEY_F7, KEY_F3,  /* out2 */
				KEY_F16, KEY_F12, KEY_F8, KEY_F4   /* out3 */
			},
		});

	platform_add_devices(b2000_devices,
		ARRAY_SIZE(b2000_devices));

	/* 1 SATA + 1 PCIe */
	stih415_configure_miphy(&(struct stih415_miphy_config) {
		/* The UNUSED is to improve resiliance to broken
		 * PCIe clock chips, you need a valid clock to the PCIe
		 * block in order to talk to the miphy. If you have a duff
		 * clock chip disabling CONFIG_PCI will at least get you
		 * working SATA
		 */
		.modes = (enum miphy_mode[2]) {
			SATA_MODE, IS_ENABLED(CONFIG_PCI) ?
				      PCIE_MODE : UNUSED_MODE },
		});
	stih415_configure_sata(0, &(struct stih415_sata_config) { });

	stih415_configure_pcie(&(struct stih415_pcie_config) {
				.reset_gpio = stm_gpio(STIH415_PIO(106), 5),
				});

	return;
}

static int __init b2000_late_devices_setup(void)
{
#if defined(CONFIG_MACH_STM_B2000_CN22_B2035) || defined(CONFIG_MACH_STM_B2000_CN22_B2032)
	return	platform_device_register(&stmmac0_mdio_gpio_bus);
#else
	return 0;
#endif
}
late_initcall(b2000_late_devices_setup);

MACHINE_START(STM_B2000, "STMicroelectronics B2000 - STiH415 MBoard")
	.atag_offset	= 0x100,
	.map_io		= stih415_map_io,
#ifdef CONFIG_SPARSE_IRQ
	.nr_irqs	= NR_IRQS_LEGACY,
#endif
	.init_early	= b2000_init_early,
	.init_irq	= mpe41_gic_init_irq,
	.timer		= &mpe41_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= b2000_init,
	.restart	= stih415_reset,
MACHINE_END

#ifdef CONFIG_HIBERNATION_ON_MEMORY

#include <linux/stm/hom.h>

static int b2000_hom_restore(void)
{
	b2000_ethphy_gpio_init(0);
	b2000_configure_tsin(0);
	return 0;
}

static int b2000_hom_freeze(void)
{
	b2000_configure_tsin(1);
	return 0;
}

static struct stm_hom_board b2000_hom = {
	.restore = b2000_hom_restore,
	.freeze = b2000_hom_freeze,
};

static int __init b2000_hom_init(void)
{
	return stm_hom_board_register(&b2000_hom);
}

late_initcall(b2000_hom_init);
#endif


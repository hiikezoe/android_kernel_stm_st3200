/*
 * Copyright (c) 2011 STMicroelectronics Limited
 * Author: Stuart Menefy <stuart.menefy@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_STM_STXH205_H
#define __LINUX_STM_STXH205_H

#include <linux/device.h>
#include <linux/phy.h>
#include <linux/stm/platform.h>

#define SYSCONFG_GROUP(x) \
	(((x) < 400) ? ((x) / 100) : 3)
#define SYSCONF_OFFSET(x) \
	(((x) < 400) ? ((x) % 100) : ((x) - 400))

#define SYSCONF(x) \
	SYSCONFG_GROUP(x), SYSCONF_OFFSET(x)

#define LPM_SYSCONF_BANK	(4)

void stxh205_early_device_init(void);

#define STXH205_ASC(x) (((x) < 10) ? (x) : ((x)-7))

struct stxh205_asc_config {
	int hw_flow_control;
	int is_console;
};
void stxh205_configure_asc(int asc, struct stxh205_asc_config *config);

struct stxh205_ssc_config {
	union {
		struct {
			enum {
				stxh205_ssc0_sclk_pio9_2,
				stxh205_ssc0_sclk_pio6_2
			} sclk;
			enum {
				stxh205_ssc0_mtsr_pio9_3,
				stxh205_ssc0_mtsr_pio6_3
			} mtsr;
			enum {
				stxh205_ssc0_mrst_pio9_7
			} mrst;
		} ssc0;
		struct {
			enum {
				stxh205_ssc1_sclk_pio4_6,
				stxh205_ssc1_sclk_pio12_0
			} sclk;
			enum {
				stxh205_ssc1_mtsr_pio4_7,
				stxh205_ssc1_mtsr_pio12_1,
			} mtsr;
			enum {
				stxh205_ssc1_mrst_pio4_5,
				stxh205_ssc1_mrst_pio11_7
			} mrst;
		} ssc1;
		struct {
			enum {
				stxh205_ssc2_sclk_pio7_6,
				stxh205_ssc2_sclk_pio8_6,
				stxh205_ssc2_sclk_pio9_4,
				stxh205_ssc2_sclk_pio_10_5
			} sclk;
			enum {
				stxh205_ssc2_mtsr_pio7_7,
				stxh205_ssc2_mtsr_pio8_7,
				stxh205_ssc2_mtsr_pio9_5,
				stxh205_ssc2_mtsr_pio10_6,
			} mtsr;
			enum {
				stxh205_ssc2_mrst_pio7_0,
				stxh205_ssc2_mrst_pio8_5,
				stxh205_ssc2_mrst_pio9_7,
				stxh205_ssc2_mrst_pio10_7
			} mrst;
		} ssc2;
		struct {
			enum {
				stxh205_ssc3_sclk_pio13_4,
				stxh205_ssc3_sclk_pio15_0,
				stxh205_ssc3_sclk_pio15_5
			} sclk;
			enum {
				stxh205_ssc3_mtsr_pio13_5,
				stxh205_ssc3_mtsr_pio15_1,
				stxh205_ssc3_mtsr_pio15_6,
			} mtsr;
			enum {
				stxh205_ssc3_mrst_pio13_6,
				stxh205_ssc3_mrst_pio15_2,
				stxh205_ssc3_mrst_pio15_7
			} mrst;
		} ssc3;
	} routing;
	void (*spi_chipselect)(struct spi_device *spi, int is_on);
	unsigned int i2c_speed;
};

#define STXH205_SSC(x)	(((x) < 10) ? (x) : ((x)-10+4))

/* Use the above macro while passing SSC number. */
int stxh205_configure_ssc_spi(int ssc, struct stxh205_ssc_config *config);
int stxh205_configure_ssc_i2c(int ssc, struct stxh205_ssc_config *config);

struct stxh205_lirc_config {
	enum {
		stxh205_lirc_rx_disabled,
		stxh205_lirc_rx_mode_ir,
		stxh205_lirc_rx_mode_uhf
	} rx_mode;
	int tx_enabled;
	int tx_od_enabled;
};
void stxh205_configure_lirc(struct stxh205_lirc_config *config);

struct stxh205_pwm_config {
	int out10_enabled;
	int out11_enabled;
};
void stxh205_configure_pwm(struct stxh205_pwm_config *config);

struct stxh205_ethernet_config {
	phy_interface_t interface;
	int no_txer;
	int ext_clk;
	int phy_bus;
	int phy_addr;
	struct stmmac_mdio_bus_data *mdio_bus_data;
};
void stxh205_configure_ethernet(struct stxh205_ethernet_config *config);

void stxh205_configure_usb(int port);

void stxh205_configure_mmc(int emmc);

void stxh205_configure_spifsm(struct stm_plat_spifsm_data *data);

void stxh205_configure_nand(struct stm_nand_config *config);

#endif

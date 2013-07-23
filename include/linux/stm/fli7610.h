/*
 * Copyright (c) 2010 STMicroelectronics Limited
 * Author: Srinivas.Kandagatla <srinivas.kandagatla@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_STM_FLI7610_H
#define __LINUX_STM_FLI7610_H

#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/stm/platform.h>



/*
 * ARM and ST40 interrupts are virtually identical, so we can use the same
 * parameter for both. Only mailbox and some A/V interrupts are connected
 * to the ST200's, however 4 ILC outputs are available, which could be
 * used if required.
 */
#if defined(CONFIG_SUPERH)
#define FLI7610_IRQ(irq) ILC_IRQ(irq)
#elif defined(CONFIG_ARM)
#define FLI7610_IRQ(irq) ((irq)+32)
#endif

#define FLI7610_RESOURCE_IRQ(_irq) \
	{ \
		.start = FLI7610_IRQ(_irq), \
		.end = FLI7610_IRQ(_irq),	\
		.flags = IORESOURCE_IRQ, \
	}

#define FLI7610_RESOURCE_IRQ_NAMED(_name, _irq)	\
	{ \
		.start = FLI7610_IRQ(_irq), \
		.end = FLI7610_IRQ(_irq), \
		.name = (_name), \
		.flags = IORESOURCE_IRQ, \
	}

#ifndef CONFIG_ARM
#define IO_ADDRESS(x) 0
#endif


/* MPE SYSCONF 400 - 686*/
#define MPE_SYSCONFG_GROUP(x) \
	(((x)/100)+1)

#define MPE_SYSCONF_OFFSET(x) \
	((x) % 100)

#define MPE_SYSCONF(x) \
	MPE_SYSCONFG_GROUP(x), MPE_SYSCONF_OFFSET(x)

/* TAE SYSCONF 0 - 473 */
#define TAE_SYSCONFG_GROUP(x) \
	(((x) < 100) ? 0 : ((((x) > 299) && ((x) <= 445)) ? 3 : (((x)/100))))

#define TAE_SYSCONF_OFFSET(x) \
	(((x) >= 450 && (x) < 474) ? ((x) - 450) : ((x) % 100))

#define TAE_SYSCONF(x) \
	TAE_SYSCONFG_GROUP(x), TAE_SYSCONF_OFFSET(x)


#define FLI7610_PIO(x) \
	(((x) < 100) ? (x) : ((x)-100+19))

#define LPM_SYSCONF_BANK	(8)
#define LPM_SYSCONF(x) LPM_SYSCONF_BANK, x

void fli7610_early_device_init(void);

struct fli7610_asc_config {
	int hw_flow_control;
	int is_console;
	int force_m1;
};
void fli7610_configure_asc(int asc, struct fli7610_asc_config *config);
void fli7610_configure_usb(int port);

#define FLI7610_SBC_SSC(num)		(num + 3)
#define FLI7610_SSC(num)		(num)

/*
 * The Newman documentation is slightly confusing, because it refers to
 * I2C and SPI busses separately, rather than SSC numbers, and also it
 * is based from 1 not 0, so explicitly enumerate them here.
 */
#define FLI7610_I2C1		FLI7610_SSC(0)
#define FLI7610_I2C2		FLI7610_SSC(1)
#define FLI7610_I2C3		FLI7610_SSC(2)
#define FLI7610_I2C1_LPM	FLI7610_SBC_SSC(0)
#define FLI7610_SPI3		FLI7610_SSC(2)
#define FLI7610_SPI1_LPM	FLI7610_SBC_SSC(1)

struct fli7610_ssc_config {
	void (*spi_chipselect)(struct spi_device *spi, int is_on);
	unsigned int i2c_speed;
};
/* Use the above macros while passing SSC number. */
int fli7610_configure_ssc_spi(int ssc, struct fli7610_ssc_config *config);
int fli7610_configure_ssc_i2c(int ssc, struct fli7610_ssc_config *config);
void fli7610_configure_lirc(void);
struct fli7610_pwm_config {
	enum {
		fli7610_tae_pwm = 0,
		fli7610_sbc_pwm
	} pwm;
	int enabled[STM_PLAT_PWM_NUM_CHANNELS];
};
void fli7610_configure_pwm(struct fli7610_pwm_config *config);

void fli7610_reset(char mode, const char *cmd);

struct fli7610_audio_config {
	enum {
		fli7610_uni_player_0_pcm_disabled,
		fli7610_uni_player_0_pcm_2_channels,
		fli7610_uni_player_0_pcm_4_channels,
		fli7610_uni_player_0_pcm_6_channels,
		fli7610_uni_player_0_pcm_8_channels,
	} uni_player_0_pcm_mode;
	enum {
		fli7610_uni_player_1_pcm_disabled,
		fli7610_uni_player_1_pcm_2_channels,
	} uni_player_1_pcm_mode;
	int uni_player_4_spdif_enabled;
	int uni_reader_0_spdif_enabled;
};
void fli7610_configure_audio(struct fli7610_audio_config *config);

void fli7610_configure_nand(struct stm_nand_config *config);

void fli7610_configure_spifsm(struct stm_plat_spifsm_data *data);

void fli7610_configure_mmc(int emmc);

struct fli7610_miphy_config {
	int force_jtag;
	enum miphy_mode *modes;
	int tx_pol_inv;
	int rx_pol_inv;
};
void fli7610_configure_miphy(struct fli7610_miphy_config *config);

struct fli7610_pcie_config {
	unsigned reset_gpio; /* Which (if any) gpio for PCIe reset */
	void (*reset)(void); /* Do something else on reset if needed */
};

void fli7610_configure_pcie(struct fli7610_pcie_config *config);

#endif

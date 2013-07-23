/*
 * Copyright (C) 2012 STMicroelectronics Limited
 *
 * Author(s): Carmelo Amoroso <carmelo.amoroso@st.com>
 *            Francesco Virlinzi <francesco.virlinzi@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/stm/platform.h>
#include <linux/stm/stig125.h>

#include <asm/mach-types.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/cache-l2x0.h>

#include <mach/soc-stig125.h>
#include <mach/hardware.h>

static struct platform_device b2044_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev.platform_data = &(struct gpio_led_platform_data) {
		.num_leds = 1,
		.leds = (struct gpio_led[]) {
			{
				.name = "FP_LED",
				.default_trigger = "heartbeat",
				.gpio = stm_gpio(STIG125_SBC_PIO(2), 2),
			}
		},
	},
};

static struct platform_device *b2044_devices[] __initdata = {
	&b2044_leds,
};

static void __init b2044_init(void)
{
	platform_add_devices(b2044_devices, ARRAY_SIZE(b2044_devices));

	stig125_configure_lirc(&(struct stig125_lirc_config) {
			.rx_mode = stig125_lirc_rx_mode_ir, });

	stig125_configure_usb(0);
	stig125_configure_usb(1);
	stig125_configure_usb(2);
	stig125_configure_fp();

#ifdef CONFIG_MACH_STM_B2044_B2048_EMMC
	stig125_configure_mmc(1);
#elif defined(CONFIG_MACH_STM_B2044_B2048_SLOT)
	stig125_configure_mmc(0);
#endif

	stig125_configure_miphy(&(struct stig125_miphy_config){
			.id = 0,
			.mode = SATA_MODE,
			.iface = UPORT_IF,
			});

	sti125_configure_sata(0);

	stig125_configure_miphy(&(struct stig125_miphy_config){
			.id = 1,
			.mode = SATA_MODE,
			.iface = UPORT_IF,
			.tx_pol_inv = 1,
			});

	sti125_configure_sata(1);

	stig125_configure_ssc_i2c(STIG125_TELSS_SSC, 100);
	stig125_configure_ssc_i2c(STIG125_HDMI_SSC, 100);
	stig125_configure_ssc_i2c(STIG125_FE_SSC, 100);
	stig125_configure_ssc_i2c(STIG125_BE_SSC, 100);

}

static void __init b2044_init_early(void)
{
	printk(KERN_INFO "STMicroelectronics STiG125 (Barcelona) MBoard initialisation\n");

	stig125_early_device_init();

	stig125_configure_asc(STIG125_SBC_ASC(0), &(struct stig125_asc_config) {
			.hw_flow_control = 0,
			.is_console = 1,
			.force_m1 = 1, });
}

MACHINE_START(STM_B2044, "STMicroelectronics B2044 - STiG125 board")
	.atag_offset    = 0x100,
	.map_io		= stig125_map_io,
#ifdef CONFIG_SPARSE_IRQ
	.nr_irqs        = NR_IRQS_LEGACY,
#endif
	.init_early	= b2044_init_early,
	.init_irq       = stig125_gic_init_irq,
	.timer		= &stig125_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= b2044_init,
MACHINE_END

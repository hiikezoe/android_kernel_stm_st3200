/*
 * (c) 2010,2011 STMicroelectronics Limited
 *
 * Authors:
 *   Pawel Moll <pawel.moll@st.com>
 *   Stuart Menefy <stuart.menefy@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * For more details on the retiming logic which this code controls, see
 *    Generic Retime Padlogic functional spec (ADCS 8198257)
 * and
 *    STi7108 Generic Retime Padlogic Application Note
 */

#include <linux/stm/sysconf.h>
#include "pio-control.h"


static void stm_pio_control_config_direction(
		struct stm_pio_control *pio_control,
		int pin, enum stm_pad_gpio_direction direction)
{
	struct sysconf_field *output_enable;
	struct sysconf_field *pull_up;
	struct sysconf_field *open_drain;
	unsigned long oe_value, pu_value, od_value;
	unsigned long mask;

	pr_debug("%s(pin=%d, direction=%d)\n",
			__func__, pin, direction);

	output_enable = pio_control->oe;
	pull_up = pio_control->pu;
	open_drain = pio_control->od;

	mask = 1 << pin;

	oe_value = sysconf_read(output_enable);
	pu_value = sysconf_read(pull_up);
	od_value = sysconf_read(open_drain);

	switch (direction) {
	case stm_pad_gpio_direction_in:
		/* oe = 0, pu = 0, od = 0 */
		oe_value &= ~mask;
		pu_value &= ~mask;
		od_value &= ~mask;
		break;
	case stm_pad_gpio_direction_in_pull_up:
		/* oe = 0, pu = 1, od = 0 */
		oe_value &= ~mask;
		pu_value |= mask;
		od_value &= ~mask;
		break;
	case stm_pad_gpio_direction_out:
		/* oe = 1, pu = 0, od = 0 */
		oe_value |= mask;
		pu_value &= ~mask;
		od_value &= ~mask;
		break;
	case stm_pad_gpio_direction_bidir:
		/* oe = 1, pu = 0, od = 1 */
		oe_value |= mask;
		pu_value &= ~mask;
		od_value |= mask;
		break;
	case stm_pad_gpio_direction_bidir_pull_up:
		/* oe = 1, pu = 1, od = 1 */
		oe_value |= mask;
		pu_value |= mask;
		od_value |= mask;
		break;
	case stm_pad_gpio_direction_ignored:
		return;
	default:
		BUG();
		break;
	}

	sysconf_write(output_enable, oe_value);
	sysconf_write(pull_up, pu_value);
	sysconf_write(open_drain, od_value);
}

static void stm_pio_control_config_function(
		struct stm_pio_control *pio_control,
		int pin, int function)
{
	struct sysconf_field *selector;
	int offset;
	unsigned long val;

	pr_debug("%s(pin=%d, function=%d)\n",
			__func__, pin, function);

	selector = pio_control->alt;

	offset = pin * 4;

	val = sysconf_read(selector);
	val &= ~(0xf << offset);
	val |= function << offset;
	sysconf_write(selector, val);
}

static unsigned long stm_pio_control_delay_to_bit(unsigned int delay,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction)
{
	unsigned int *delay_times;
	int num_delay_times;
	int i;
	unsigned int closest_divergence = UINT_MAX;
	int closest_index = -1;

	switch (direction) {
	case stm_pad_gpio_direction_in:
		delay_times = retime_params->delay_times_in;
		num_delay_times = retime_params->num_delay_times_in;
		break;
	case stm_pad_gpio_direction_out:
		delay_times = retime_params->delay_times_out;
		num_delay_times = retime_params->num_delay_times_out;
		break;
	default:
		WARN(delay, "Attempt to set delay without knowing direction");
		return 0;
	}

	for (i = 0; i < num_delay_times; i++) {
		unsigned int divergence = abs(delay - delay_times[i]);

		if (divergence == 0)
			return i;

		if (divergence < closest_divergence) {
			closest_divergence = divergence;
			closest_index = i;
		}
	}

	WARN(1, "Attempt to set delay %d, closest available %d\n",
	     delay, delay_times[closest_index]);

	return closest_index;
}

static void stm_pio_control_config_retime_packed(
		struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, const struct stm_pio_control_retime_config *rt)
{
	const struct stm_pio_control_retime_offset *offset
		= retime_params->retime_offset;
	struct sysconf_field **regs;
	unsigned long values[2];
	unsigned long mask;
	int i, j;

	unsigned long delay = stm_pio_control_delay_to_bit(
		rt->delay, retime_params, direction);

	unsigned long retime_config =
		((rt->clk          & 1) << offset->clk1notclk0_offset) |
		((rt->clknotdata   & 1) << offset->clknotdata_offset) |
		((delay            & 1) << offset->delay_lsb_offset) |
		(((delay >> 1)     & 1) << offset->delay_msb_offset) |
		((rt->double_edge  & 1) << offset->double_edge_offset) |
		((rt->invertclk    & 1) << offset->invertclk_offset) |
		((rt->retime       & 1) << offset->retime_offset);

	pr_debug("%s(pin=%d, retime_config=%02lx)\n",
		 __func__, pin, retime_config);

	regs = pio_control->retiming;

	values[0] = sysconf_read(regs[0]);
	values[1] = sysconf_read(regs[1]);

	for (i = 0; i < 2; i++) {
		mask = 1 << pin;
		for (j = 0; j < 4; j++) {
			if (retime_config & 1)
				values[i] |= mask;
			else
				values[i] &= ~mask;
			mask <<= 8;
			retime_config >>= 1;
		}
	}

	sysconf_write(regs[0], values[0]);
	sysconf_write(regs[1], values[1]);
}

static void stm_pio_control_config_retime_dedicated(
		struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, const struct stm_pio_control_retime_config *rt)
{
	struct sysconf_field *reg;

	unsigned long delay = stm_pio_control_delay_to_bit(
		rt->delay, retime_params, direction);

	unsigned long retime_config =
		((rt->clk            & 0x3) << 0) |
		((rt->clknotdata     & 0x1) << 2) |
		((delay              & 0xf) << 3) |
		((direction == stm_pad_gpio_direction_in) ? (1 << 7) : 0) |
		((rt->double_edge    & 0x1) << 8) |
		((rt->invertclk      & 0x1) << 9) |
		((rt->retime         & 0x1) << 10);

	pr_debug("%s(pin=%d, retime_config=%02lx)\n",
		 __func__, pin, retime_config);

	reg = pio_control->retiming[pin];

	sysconf_write(reg, retime_config);
}

#ifdef CONFIG_DEBUG_FS

static int stm_pio_control_report_direction(struct stm_pio_control *pio_control,
		int pin, char *buf, int len,
		enum stm_pad_gpio_direction *direction)
{
	unsigned long oe_value, pu_value, od_value;

	oe_value = sysconf_read(pio_control->oe);
	pu_value = sysconf_read(pio_control->pu);
	od_value = sysconf_read(pio_control->od);

	if (direction)
		*direction = oe_value ? stm_pad_gpio_direction_out :
			stm_pad_gpio_direction_in;

	return snprintf(buf, len, "oe %ld, pu %ld, od %ld",
		(oe_value >> pin) & 1,
		(pu_value >> pin) & 1,
		(od_value >> pin) & 1);
}

static int stm_pio_control_report_retime_packed(
		struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, char *buf, int len)
{
	const struct stm_pio_control_retime_offset *offset
		= retime_params->retime_offset;
	unsigned long rt_value[2];
	unsigned long rt_reduced;
	int i, j;

	rt_value[0] = sysconf_read(pio_control->retiming[0]);
	rt_value[1] = sysconf_read(pio_control->retiming[1]);

	rt_reduced = 0;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++) {
			if (rt_value[i] & (1<<((8*j)+pin)))
				rt_reduced |= 1 << ((i*4)+j);
		}
	}

	return snprintf(buf, len,
		 "rt %ld, c1nc0 %ld, cnd %ld, de %ld, ic %ld, dly %ld%ld",
		 (rt_reduced >> offset->retime_offset) & 1,
		 (rt_reduced >> offset->clk1notclk0_offset) & 1,
		 (rt_reduced >> offset->clknotdata_offset) & 1,
		 (rt_reduced >> offset->double_edge_offset) & 1,
		 (rt_reduced >> offset->invertclk_offset) & 1,
		 (rt_reduced >> offset->delay_msb_offset) & 1,
		 (rt_reduced >> offset->delay_lsb_offset) & 1);
}

static int stm_pio_control_report_retime_dedicated(
		struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, char *buf, int len)
{
	unsigned long value;

	value = sysconf_read(pio_control->retiming[pin]);

	return snprintf(buf, len,
		"clk %ld, cnd %ld, dly %ld, dino %ld, de %ld, ic %ld, rt %ld",
		((value >> 0) & 0x3),
		((value >> 2) & 0x1),
		((value >> 3) & 0xf),
		((value >> 7) & 0x1),
		((value >> 8) & 0x1),
		((value >> 9) & 0x1),
		((value >> 10) & 0x1));
}

#endif /* CONFIG_DEBUG_FS */

void __init stm_pio_control_init(const struct stm_pio_control_config *config,
		struct stm_pio_control *pio_control, int num)
{
	unsigned int i, j;

	for (i=0; i<num; i++) {
		pio_control[i].config = &config[i];
		pio_control[i].alt = sysconf_claim(config[i].alt.group,
			config[i].alt.num, 0, 31,
			"PIO Alternative Function Selector");
		if (!pio_control[i].alt) goto failed;

		pio_control[i].oe = sysconf_claim(config[i].oe.group,
			config[i].oe.num, config[i].oe.lsb, config[i].oe.msb,
			"PIO Output Enable Control");
		if (!pio_control[i].oe) goto failed;
		pio_control[i].pu = sysconf_claim(config[i].pu.group,
			config[i].pu.num, config[i].pu.lsb, config[i].pu.msb,
			"PIO Pull Up Control");
		if (!pio_control[i].pu) goto failed;
		pio_control[i].od = sysconf_claim(config[i].od.group,
			config[i].od.num, config[i].od.lsb, config[i].od.msb,
			"PIO Open Drain Control");
		if (!pio_control[i].od) goto failed;

		switch (config[i].retime_style) {
		case stm_pio_control_retime_style_none:
			break;
		case stm_pio_control_retime_style_packed:
			for (j = 0; j < 2; j++) {
				pio_control[i].retiming[j] = sysconf_claim(
					config[i].retiming[j].group,
					config[i].retiming[j].num, 0, 31,
					"PIO Retiming Configuration");
				if (!pio_control[i].retiming[j])
					goto failed;
			}
			break;
		case stm_pio_control_retime_style_dedicated:
			for (j = 0; j < 8; j++)
				if ((1<<j) & config[i].retime_pin_mask) {
					pio_control[i].retiming[j] =
						sysconf_claim(
						config[i].retiming[j].group,
						config[i].retiming[j].num,
						0, 10,
						"PIO Retiming Configuration");
					if (!pio_control[i].retiming[j])
						goto failed;
				}
			break;
		}
	}

	return;

failed:
	/* Can't do anything is early except panic */
	panic("Unable to allocate PIO control sysconfs");
}

/*
 * All the above functions only need basic sysconf capability.
 * However these are aware of the way PIO hardware is also involved
 * in pad control.
 */

#include <linux/stm/gpio.h>

static void (*const config_retime_fn[])(struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, const struct stm_pio_control_retime_config *rt) = {
	[stm_pio_control_retime_style_none] = NULL,
	[stm_pio_control_retime_style_packed] = stm_pio_control_config_retime_packed,
	[stm_pio_control_retime_style_dedicated] = stm_pio_control_config_retime_dedicated,
};

int stm_pio_control_config_all(unsigned gpio,
		enum stm_pad_gpio_direction direction, int function,
		struct stm_pio_control_pad_config *config,
		struct stm_pio_control *pio_controls,
		int num_gpios, int num_functions)
{
	int port = stm_gpio_port(gpio);
	int pin = stm_gpio_pin(gpio);
	struct stm_pio_control *pio_control;
	const struct stm_pio_control_config *pio_control_config;
	const struct stm_pio_control_retime_config no_retiming = {
		.retime = 0,
		.clknotdata = 0,
		.delay = 0,
	};

	BUG_ON(port >= num_gpios);
	BUG_ON(function < 0 || function >= num_functions);

	pio_control = &pio_controls[port];
	pio_control_config = pio_control->config;

	if (function == 0) {
		switch (direction) {
		case stm_pad_gpio_direction_in:
			stm_gpio_direction(gpio, STM_GPIO_DIRECTION_IN);
			break;
		case stm_pad_gpio_direction_out:
			stm_gpio_direction(gpio, STM_GPIO_DIRECTION_OUT);
			break;
		case stm_pad_gpio_direction_bidir:
			stm_gpio_direction(gpio, STM_GPIO_DIRECTION_BIDIR);
			break;
		default:
			BUG();
			break;
		}
	} else {
		stm_pio_control_config_direction(pio_control, pin, direction);
	}

	stm_pio_control_config_function(pio_control, pin, function);

	if (config_retime_fn[pio_control_config->retime_style] &&
	    ((1 << pin) & pio_control_config->retime_pin_mask)) {
		config_retime_fn[pio_control_config->retime_style](pio_control,
			pio_control_config->retime_params, direction, pin,
			(config && config->retime) ? config->retime :
				&no_retiming);
	} else {
		WARN_ON(config && config->retime);
	}

	return 0;
}

#ifdef CONFIG_DEBUG_FS

static int (*const report_retime_fn[])(struct stm_pio_control *pio_control,
		const struct stm_pio_control_retime_params *retime_params,
		enum stm_pad_gpio_direction direction,
		int pin, char *buf, int len) = {
	[stm_pio_control_retime_style_none] = NULL,
	[stm_pio_control_retime_style_packed] = stm_pio_control_report_retime_packed,
	[stm_pio_control_retime_style_dedicated] = stm_pio_control_report_retime_dedicated,
};

void stm_pio_control_report_all(int gpio,
		struct stm_pio_control *pio_controls,
		char *buf, int len)
{
	int port = stm_gpio_port(gpio);
	int pin = stm_gpio_pin(gpio);
	struct stm_pio_control *pio_control = &pio_controls[port];
	const struct stm_pio_control_config *pio_control_config = pio_control->config;
	unsigned long alt_value;
	unsigned long function;
	int off;
	enum stm_pad_gpio_direction direction = stm_pad_gpio_direction_unknown;

	alt_value = sysconf_read(pio_control->alt);
	function = (alt_value >> (pin * 4)) & 0xf;
	off = snprintf(buf, len, "alt fn %ld - ", function);

	if (function == 0)
		off += snprintf(buf+off, len-off, "%s",
			stm_gpio_get_direction(gpio));
	else
		off += stm_pio_control_report_direction(pio_control,
			pin, buf+off, len-off, &direction);

	if (report_retime_fn[pio_control_config->retime_style] &&
	    ((1 << pin) & pio_control_config->retime_pin_mask)) {
		off += snprintf(buf+off, len-off, " - ");
		off += report_retime_fn[pio_control_config->retime_style](
			pio_control, pio_control_config->retime_params,
			direction, pin, buf+off, len-off);
	}
}

#endif

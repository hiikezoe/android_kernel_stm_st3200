/*---------------------------------------------------------------------------
* /include/linux/stm/lpm.h
* Copyright (C) 2011 STMicroelectronics Limited
* Contributor:Francesco Virlinzi <francesco.virlinzi@st.com>
* Author:Pooja Agarwal <pooja.agarwal@st.com>
* Author:Udit Kumar <udit-dlh.kumar@st.cm>
* May be copied or modified under the terms of the GNU General Public
* License.  See linux/COPYING for more information.
*----------------------------------------------------------------------------
*/
#ifndef __LPM_H
#define __LPM_H

#include <linux/rtc.h>

/*
 * stm_lpm_wakeupdevices
 *
 * Define wakeup devices
 */
enum stm_lpm_wakeup_devices {
	STM_LPM_WAKEUP_IR  = 1<<0,
	STM_LPM_WAKEUP_CEC = 1<<1,
	STM_LPM_WAKEUP_FRP = 1<<2,
	STM_LPM_WAKEUP_WOL = 1<<3,
	STM_LPM_WAKEUP_RTC = 1<<4,
	STM_LPM_WAKEUP_ASC = 1<<5,
	STM_LPM_WAKEUP_NMI = 1<<6,
	STM_LPM_WAKEUP_HPD = 1<<7,
	STM_LPM_WAKEUP_PIO = 1<<8,
	STM_LPM_WAKEUP_EXT = 1<<9
};

/*
 * stlpm_resettype_e
 *
 * Define reset type
 */
enum stm_lpm_reset_type {
	STM_LPM_SOC_RESET = 0,
	STM_LPM_LPM_RESET = 1<<0,
	STM_LPM_BOOT_RESET = 1<<1
};

/*
 * stm_lpm_sbcstate
 *
 * Define state of SBC software
 */
enum stm_lpm_sbc_state {
	STM_LPM_SBC_BOOT = 1,
	STM_LPM_SBC_WAIT = 3,
	STM_LPM_SBC_RUNNING = 4,
	STM_LPM_SBC_STANDBY = 5
};

/*
 * stlpm_version
 *
 * Defines the version information of STLPM
 */
struct stm_lpm_version {
	char major_comm_protocol;
	char minor_comm_protocol;
	char major_soft;
	char minor_soft;
	char patch_soft;
	char month;
	char day;
	char year;
};

/*
 * stlpm_fpsetting
 *
 * Defines the frontpanel display settings
 */
struct stm_lpm_fp_setting {
	char owner;
	char am_pm;
	char brightness;
};

/*
 * stm_lpm_PIO_LEVE
 *
 * Define state of SBC software
 */
enum stm_lpm_pio_level {
	STM_LPM_PIO_LOW = 0,
/* Interrupt/Power off  will be generated when bit goes 1 to 0*/
	STM_LPM_PIO_HIGH = 1 << 7
/* Interrupt/Power off  will be generated when bit goes 0 to 1*/
};

enum stm_lpm_pio_direction {
	STM_LPM_PIO_INPUT = 0,
	STM_LPM_PIO_OUTPUT = 1<<5

};

enum stm_lpm_pio_use {
	STM_LPM_PIO_POWER = 1,
	STM_LPM_PIO_ETH_MDINT = 2,
	STM_LPM_PIO_WAKEUP = 3,
	STM_LPM_PIO_RES = 4,

};

/*
 * stlpm_PIO_Setting
 *
 * Defines the frontpanel display settings
 */
struct stm_lpm_pio_setting {
	enum stm_lpm_pio_level pio_level;
	char interrupt_enabled; /* This is valid if PIO is used as interrupt*/
	enum stm_lpm_pio_direction pio_direction;
	enum stm_lpm_pio_use  pio_use;
	char pio_bank;
	char pio_pin;
};

enum stm_lpm_adv_feature_name {
	STM_LPM_USE_EXT_VCORE = 1,
	STM_LPM_USE_INT_VOLT_DETECT = 2,
	STM_LPM_EXT_CLOCK = 3,
	STM_LPM_RTC_SOURCE = 4,
	STM_LPM_WU_TRIGGERS = 5,
};

/* advanced features */
struct stm_lpm_adv_feature {
	enum stm_lpm_adv_feature_name feature_name;
	unsigned char feature_parameter[2];
	/* 0 for STM_LPM_USE_EXT_VCORE and STM_LPM_USE_INT_VOLT_DETECT */
	/* for STM_LPM_EXT_CLOCK use 1 for EXTERNAL,*/
	/*  2 for AGC_EXTERNAL, 3 for Track_32K*/
	/* for STM_LPM_RTC_SOURCE use 1 for RTC_32K_TCXO, 2 for RTC_32K_OSC*/
	/* for STM_LPM_WU_TRIGGERS use same values*/
	/* as defined in stm_lpm_wakeup_devices*/
};

/*
 * stlpm_irsetting
 *
 * Defines IR settings
 */
#define MAX_IR_FIFO_DEPTH 64
struct stm_lpm_ir_fifo {
	u16 mark;
	u16 symbol;
};

struct stm_lpm_ir_key {
	u8 key_index;	/*Index of key*/
	u8 num_patterns; /*64 is max value*/
	struct  stm_lpm_ir_fifo fifo[MAX_IR_FIFO_DEPTH];
};
struct stm_lpm_ir_keyinfo {

	u8 ir_id; /* device id*/
	u16 time_period; /* time period of protocol*/
	u16 time_out;
	u8 tolerance; /* By default 10% */
	struct stm_lpm_ir_key ir_key;
};

int stm_lpm_configure_wdt(u16 time_in_ms);

int stm_lpm_get_fw_state(enum stm_lpm_sbc_state *fw_state);

int stm_lpm_get_wakeup_device(enum stm_lpm_wakeup_devices *wakeupdevice);

int stm_lpm_get_wakeup_info(enum stm_lpm_wakeup_devices *wakeupdevice,
	int *validsize, int datasize, char  *data) ;

int stm_lpm_get_version(struct stm_lpm_version *drv_ver,
	struct stm_lpm_version *fw_ver);

int stm_lpm_reset(enum stm_lpm_reset_type reset_type);

int stm_lpm_setup_fp(struct stm_lpm_fp_setting *fp_setting);

int stm_lpm_setup_pio(struct stm_lpm_pio_setting *pio_setting);

int stm_lpm_setup_ir(u8 num_keys, struct stm_lpm_ir_keyinfo *keys);

int stm_lpm_set_rtc(struct rtc_time *new_rtc);

int stm_lpm_setup_keyscan(short int key_data);

int stm_lpm_set_adv_feature(unsigned char enabled,
			struct stm_lpm_adv_feature *feature);

int stm_lpm_get_adv_feature(unsigned char all_features, char *features);


int stm_lpm_set_wakeup_device(unsigned char wakeup_devices);

int stm_lpm_set_wakeup_time(u32 timeout);

int stm_lpm_get_trigger_data(enum stm_lpm_wakeup_devices wakeup_device,
	 unsigned int size_max,
	 unsigned int size_min,
	 char *data);
#endif /*__LPM_H*/

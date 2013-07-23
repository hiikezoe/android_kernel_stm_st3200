/* ---------------------------------------------------------------------------
 * Copyright (C) 2012 STMicroelectronics Limited
 *
 * Author: Pooja Agarwal <pooja.agarwal@st.com>
 * Author: Udit Kumar <udit-dlh.kumar@st.cm>
 * Contributor: Francesco Virlinzi <francesco.virlinzi@st.com>
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 * ----------------------------------------------------------------------------
 */
#include <linux/stm/lpm.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/stm/platform.h>
#include "lpm_def.h"

#ifdef CONFIG_STM_LPM_DEBUG
#define lpm_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define lpm_debug(fmt, ...)
#endif

/************************************************************************/
/* Purpose : To get driver and firmware version Message format          */
/* Protocol version of both driver and firmware should be same          */
/* Host will ask firmware version by following command                  */
/* | byte0      |   byte1          |                                    */
/* | command id |   Transaction id |                                    */
/* | command id = STM_LPM_MSG_VER                                       */
/* SBC reply will be                                                    */
/* | byte0  |byte1 |    byte2   |byte3  | byte4 | byte 5                */
/* Where byte0 is reply STM_LPM_MSG_VER| STM_LPM_MSG_REPLY              */
/* byte 1 Transaction id                                                */
/* byte2[7-4] Major and byte2[3-0] minor protocol num                   */
/* byte3[7-4] Major and byte3[3-0] minor software version               */
/* byte4[7-4] software patch and byte4[3-0] software release build month*/
/* byte5 software release build day                                     */
/* byte6 software release build year                                    */
/************************************************************************/

int stm_lpm_get_version(struct stm_lpm_version *driver_version,
	struct stm_lpm_version *fw_version)
{
	int err = 0;
	struct stm_lpm_message response = {0};
	struct stlpm_internal_send_msg	send_msg;
	/* check paramters */
	if ((driver_version == NULL) || (fw_version == NULL))
		return -EINVAL;

	/*fill data into internal message*/
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_VER, NULL, MSG_ZERO_SIZE);

	/*fill transaction ID and reply type*/
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);

	err = lpm_exchange_msg(&send_msg, &response);
	/* check we got no error from SBC and response is as expected */
	if (err >= 0 && (response.command_id & STM_LPM_MSG_REPLY)) {
		/*Copy the received data to user space */
		fw_version->major_comm_protocol = response.msg_data[0] >> 4;
		fw_version->minor_comm_protocol = response.msg_data[0] & 0x0F;
		fw_version->major_soft = response.msg_data[1] >> 4;
		fw_version->minor_soft = response.msg_data[1] & 0x0F;
		fw_version->patch_soft = response.msg_data[2] >> 4;
		fw_version->month = response.msg_data[2] & 0x0F;
		memcpy(&fw_version->day, &response.msg_data[3], 2);

		driver_version->major_comm_protocol = STM_LPM_MAJOR_PROTO_VER;
		driver_version->minor_comm_protocol = STM_LPM_MINOR_PROTO_VER;
		driver_version->major_soft = STM_LPM_MAJOR_SOFT_VER;
		driver_version->minor_soft = STM_LPM_MINOR_SOFT_VER;
		driver_version->patch_soft = STM_LPM_PATCH_SOFT_VER;
		driver_version->month = STM_LPM_BUILD_MONTH;
		driver_version->day = STM_LPM_BUILD_DAY;
		driver_version->year = STM_LPM_BUILD_YEAR;
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_version);

/************************************************************************/
/* Purpose : To set watchdog time out for SBC firmware                  */
/* Host will use  following command                                     */
/* | byte0  |byte1  |   byte2   |   byte3  |                            */
/* | command id |   Transaction id |                                    */
/* | command id = STM_LPM_MSG_SET_WDT                                   */
/* byte 2 and byte 3 will contain wdt timeout of SBC                    */
/* SBC reply will be                                                    */
/* | byte0  |byte1                                                      */
/* Where byte0 is reply STM_LPM_MSG_SET_WDT| STM_LPM_MSG_REPLY          */
/* byte 1 Transaction id                                                */
/************************************************************************/

int stm_lpm_configure_wdt(u16 time_in_ms)
{
	char msg[2] = {0};
	struct stlpm_internal_send_msg	send_msg = {0};
	struct stm_lpm_message response;
	if (!time_in_ms)
		return -EINVAL;
	/*since other machine always in litte endian mode */
	/* then msg[0] will always be LSB */
	msg[1] = (time_in_ms >> 8) & BYTE_MASK;
	msg[0] = time_in_ms  & BYTE_MASK;
	/*Send message for SBC */
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_WDT, msg, 2);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	return lpm_exchange_msg(&send_msg, &response);
}
EXPORT_SYMBOL(stm_lpm_configure_wdt);

/************************************************************************/
/* Purpose : To get current state of sbc firmware                       */
/* Host will use  following command                                     */
/* | byte0      |    byte1            |                                 */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_GET_STATUS                                */
/* SBC reply will be                                                    */
/* | byte0  |byte1       |    byte2                                     */
/* Where byte0 is reply STM_LPM_MSG_GET_STATUS| STM_LPM_MSG_REPLY       */
/* byte 1 Transaction id                                                */
/* byte 2 current status of firmware as  in enum STM_LPM_MSG_GET_STATUS */
/************************************************************************/

int stm_lpm_get_fw_state(enum stm_lpm_sbc_state *fw_state)
{
	int err = 0;
	struct stlpm_internal_send_msg send_msg;
	struct stm_lpm_message reply_msg = {0};
	if (fw_state == NULL)
		return -EINVAL;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_GET_STATUS, NULL, MSG_ZERO_SIZE);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &reply_msg);
	/* check we got no error from SBC and response is as expected */
	if (err >= 0 && (reply_msg.command_id & STM_LPM_MSG_REPLY))
		*fw_state = reply_msg.msg_data[0];
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_fw_state);


/************************************************************************/
/* Purpose : To set wakeup devices                                      */
/* This API will be called by Linux PM in no irq mode                   */
/* Host will use  following command                                     */
/* | byte0  |byte1  |    byte2     |    byte3  |                        */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_SET_WUD                                   */
/* SBC reply will be                                                    */
/* | byte0  |byte1       |    byte2                                     */
/* Where byte0 is reply STM_LPM_MSG_SET_WUD| STM_LPM_MSG_REPLY         */
/* byte 1 Transaction id                                                */
/************************************************************************/
int stm_lpm_set_wakeup_device(unsigned char devices)
{
	struct stm_lpm_adv_feature feature;
	feature.feature_name = STM_LPM_WU_TRIGGERS;
	feature.feature_parameter[0] = devices;
	feature.feature_parameter[1] = 3;
	return stm_lpm_set_adv_feature(1, &feature);
}
EXPORT_SYMBOL(stm_lpm_set_wakeup_device);

/************************************************************************/
/* Purpose : To set wakeup time                                         */
/* caller either by rtc-sbc or some other modules in k space            */
/* Host will use  following command                                     */
/* | byte0  |byte1        |    byte2     |    byte3  |                  */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_SET_WUD                                   */
/* Byte 2 and Byte 3 wakeup time out                                    */
/* SBC reply will be                                                    */
/* | byte0  |byte1       |    byte2                                     */
/* Where byte0 is reply STM_LPM_MSG_SET_WUD| STM_LPM_MSG_REPLY          */
/* byte 1 Transaction id                                                */
/************************************************************************/
int stm_lpm_set_wakeup_time(u32 timeout)
{
	int err = 0;
	char msg[4] = {0};
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	/*  msg[0] will always be LSB */
	if (is_bigendian())
		timeout = lpm_reverse_integer(timeout);
	/*copy timeout into message */
	memcpy(msg, &timeout, 4);
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_TIMER, msg, 4);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_set_wakeup_time);

/************************************************************************/
/* Purpose : To set rtc time                                            */
/* caller either by rtc-sbc or some other modules in k space            */
/* Host will use  following command                                     */
/* | byte0      |    byte1        |byte2 |    byte3  | byte4    byte5   */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_SET_RTC                                   */
/* Byte 2 hour, Byte 3 min, byte 4 seconds                              */
/* SBC reply will be                                                    */
/* | byte0  |byte1                                                      */
/* Where byte0 is reply STM_LPM_MSG_SET_RTC| STM_LPM_MSG_REPLY          */
/* byte 1 Transaction id                                                */
/************************************************************************/

int stm_lpm_set_rtc(struct rtc_time *new_rtc)
{
	int err = 0;
	char msg[3] = {0};
	struct stm_lpm_message response = {0};
	struct stlpm_internal_send_msg send_msg = {0};
	/*copy received values of rtc into message */
	msg[2] = new_rtc->tm_sec;
	msg[1] = new_rtc->tm_min;
	msg[0] = new_rtc->tm_hour;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_RTC, msg, 3);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_set_rtc);

/************************************************************************/
/* Purpose : To get wake devcies                                        */
/* This API will be called by Linux PM in no irq mode                   */
/* Host will use  following command                                     */
/* | byte0      |    byte1                                              */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_GET_WUD                                   */
/* SBC reply will be                                                    */
/* | byte0  |byte1     | byte 2                                         */
/* Where byte0 is reply STM_LPM_MSG_GET_WUD| STM_LPM_MSG_REPLY          */
/* byte 1 Transaction id                                                */
/* byte 2 actual wakeup device                                          */
/************************************************************************/

int stm_lpm_get_wakeup_device(enum stm_lpm_wakeup_devices *wakeup_device)
{
	char feature[10];
	int err = 0;
	err = stm_lpm_get_adv_feature(0, feature);
	if (err >= 0) {
		*wakeup_device = feature[4] | feature[5] << 8;
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_wakeup_device);

/************************************************************************/
/* Purpose : To set front panel                                         */
/* This API will be called by some kernel module                        */
/* Host will use  following command                                     */
/* | byte0      |    byte1     | byte 2                                 */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_SET_FP                                    */
/* byte2[0-1] owner of FP LED                                           */
/* byte2[2] AM/PM indicator                                             */
/* byte2[4-7] Brightness of LED                                         */
/* SBC reply will be                                                    */
/* | byte0  |byte1                                                      */
/* Where byte0 is reply STM_LPM_MSG_SET_FP| STM_LPM_MSG_REPLY           */
/* byte 1 Transaction id                                                */
/************************************************************************/
int stm_lpm_setup_fp(struct stm_lpm_fp_setting *fp_setting)
{
	char msg = 0;
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	if (fp_setting == NULL)
		return -EINVAL;

	msg = fp_setting->owner&OWNER_MASK;
	msg |= (fp_setting->am_pm & 1) << 2;
	msg |= (fp_setting->brightness & BRIGHT_MASK) << 4;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_FP, &msg, 1);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_setup_fp);


int stm_lpm_setup_pio(struct stm_lpm_pio_setting *pio_setting)
{
	char msg[3] = {0};
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	if (pio_setting == NULL || (pio_setting->pio_direction
		&& pio_setting->interrupt_enabled))
		return -EINVAL;

	msg[0] = pio_setting->pio_bank;
	msg[1] = pio_setting->pio_level |
	pio_setting->pio_direction |
	pio_setting->interrupt_enabled << 6 |
	pio_setting->pio_pin ;
	msg[2] = pio_setting->pio_use;

	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_PIO, msg, 3);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_setup_pio);

int stm_lpm_setup_keyscan(short int key_data)
{
	char msg[2] = {0};
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	memcpy(msg, &key_data, 2);

	LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_KEY_SCAN, msg, 2);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_setup_keyscan);

int stm_lpm_set_adv_feature(unsigned char enabled,
		struct stm_lpm_adv_feature *feature)
{
	char msg[4] = {0};
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	memcpy((msg+2), &feature->feature_parameter, 2);
	msg[0] = feature->feature_name;
	msg[1] = enabled;
	if (feature->feature_name == STM_LPM_WU_TRIGGERS) {
		LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_ADV_FEA, msg, 4);
		LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, NO_IRQ);
	} else {
		LPM_FILL_MSG(send_msg, STM_LPM_MSG_SET_ADV_FEA, msg, 4);
		LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	}
	err = lpm_exchange_msg(&send_msg, &response);
	return err;
}
EXPORT_SYMBOL(stm_lpm_set_adv_feature);

int stm_lpm_get_adv_feature(unsigned char all_features, char *features)
{
	char msg[1] = {0};
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	if (all_features == 1)
		msg[0] = 1;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_GET_ADV_FEA, msg, 1);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	memcpy(features, response.msg_data, 8);
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_adv_feature);

int stm_lpm_get_trigger_data(enum stm_lpm_wakeup_devices wakeup_device,
		unsigned int size_max,
		unsigned int size_min,
		char *data)
{

/*	char msg[3] = {0, 0, 0};
	int err = 0;
	struct stm_lpm_message response;
	struct stlpm_internal_send_msg send_msg = {0};
	msg[0] = wakeup_device;
	msg[1] = size_min;
	msg[2] = size_max;
	LPM_FILL_MSG(send_msg, 0x42, msg, 3);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	memcpy(data, response.msg_data, 8);
*/
	char feature[10];
	int err = 0;
	err = stm_lpm_get_adv_feature(0, feature);
	if (err >= 0) {
		data[0] = feature[6];
		data[1] = feature[7];
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_trigger_data);



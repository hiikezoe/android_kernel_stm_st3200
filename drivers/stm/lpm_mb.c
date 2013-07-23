/* ---------------------------------------------------------------------------
 *
 * Copyright (C) 2012 STMicroelectronics Limited
 *
 * Author:Pooja Agarwal <pooja.agarwal@st.com>
 * Author:Udit Kumar <udit-dlh.kumar@st.cm>
 * Contributor:Francesco Virlinzi <francesco.virlinzi@st.com>
 *
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
#include <linux/delay.h>
#include <linux/stm/platform.h>
#include <linux/firmware.h>
#include "lpm_def.h"
#include <linux/libelf.h>

#include <linux/string.h>
#include <linux/module.h>
#include <linux/elf.h>

#ifdef CONFIG_STM_LPM_DEBUG
#define lpm_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define lpm_debug(fmt, ...)
#endif

static struct platform_driver stm_sbc_driver;

/* Local struct of driver */
struct stm_lpm_driver_data {
	/* lpm_mem_base[0] is SBC program and data base address */
	/* size of lpm_mem_base[0] is 0xA0000			*/

	/* lpm_mem_base[1] is SBC mailbox address */
	/* size of lpm_mem_base[1] is 0x400 and this start from offset */
	/*0xB4000 from lpm base address */

	/* lpm_mem_base[2] is SBC configuration  address */
	/* size of lpm_mem_base[1] is 0x200 and this start from offset */
	/* 0xB5100 from lpm base address */

	/* We need to keep three addresses because in memory map of SBC */
	/* keyscan and HDMI lies in between above address range and there are */
	/* other driver which map HDMI and keyscan memory */

	void * __iomem lpm_mem_base[3];
	struct stm_lpm_message fw_reply_msg;
	struct stm_lpm_message fw_request_msg;
	char fw_name[20];
	int reply_from_sbc;
	wait_queue_head_t stm_lpm_wait_queue;
	struct mutex msg_protection_mutex;
	unsigned char glbtrans_id;
	struct work_struct lpm_sbc_reply_work;
	enum stm_lpm_sbc_state sbc_state;
	struct stm_sbc_platform_data *plat_data;
};

static struct stm_lpm_driver_data *lpm_drv;

/*Work queue to process SBC firmware request */
static void lpm_sbc_reply_worker(struct work_struct *work);

/************************************************************************/
/* Purpose : To set ir key setup                                        */
/* This API will be called by some kernel module                        */
/* This message will have more than 16 bytes,we will write in SBC memory*/
/* First message with get offset in SBC memory                          */
/* Host will fill message in SBC memory in following format             */
/* cmdid(8 bit ), trans_id, (8 bit),  device(4 bit), data_size (8bits)  */
/* time_period(16 bit),                         timeout(16bits)         */
/*tolerance(8 bits), key_index(4 bits), count(8bits), res(8 bits)       */
/*    mark0             symbol0              mark1       symbol1        */
/*    mark2             symbol2              mark3       symbol3        */
/* Message format                                                       */
/* Byte 0     Byte 1                Byte 2        Byte 3    Byte 4      */
/* Byte 0  | command id |    Byte 1 Transaction id |                    */
/* Byte 2[0-3] data_size  Byte 2[7-4] device(4 bit)                     */
/* Byte 3 data_size upper 8 buts Byte 2[7-4] device(4 bit)              */
/* Byte 4-5 time_period Byte 6-7 timeout                                */
/* Byte 8 tolerance, Byte 9[0-3] key_index Byte 10 [sequence_count ]    */
/* Byte 12 .... actual IR data                                          */
/* One write to SBC memory is done,                                     */
/* Host will send other command to SBC  to read SBC memory              */
/************************************************************************/
int stm_lpm_setup_ir(u8 num_keys, struct stm_lpm_ir_keyinfo *ir_key_info)
{
	char msg[6] = {0};
	struct stm_lpm_ir_keyinfo *this_key;
	u16 ir_size;
	u8 data;
	int offset, orginal_offset = 0, count, i, err = 0;
	struct stlpm_internal_send_msg	send_msg = {0};
	struct stm_lpm_message resp = {0};

	for (count = 0; count < num_keys; count++) {
		struct stm_lpm_ir_key *key_info;
		this_key = ir_key_info;

		ir_key_info++;
		key_info = &this_key->ir_key;
		/*Check key crediantials*/
		if (this_key->time_period == 0 || key_info->num_patterns >= 64)
			return -EINVAL;

		/* Get the buffer in lpm memory to copy this key data*/
		/*12 is fixed header to this message */
		/*each pattern will have 1 byte for mark and 1 byte for symbol*/
		ir_size = key_info->num_patterns*2 + 12;
		/*max expected value is 140*/
		msg[0] = ir_size & 0xFF;
		msg[1] = 0;
		LPM_FILL_MSG(send_msg, STM_LPM_MSG_LGWR_OFFSET, msg, 2);
		LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
		err = lpm_exchange_msg(&send_msg, &resp);
		if (err >= 0 && resp.command_id != STM_LPM_MSG_ERR) {
			/* Get the offset in SBC memory */
			offset = resp.msg_data[2] | resp.msg_data[3] << 8;
			offset |= resp.msg_data[4] << 16;
			offset |= resp.msg_data[5] << 24;
			orginal_offset = offset;

			/* Write large messg in DMEM*/
			/*command id */
			data = STM_LPM_MSG_SET_IR;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;

			 /*trans id*/
			lpm_write8(lpm_drv, DATA_OFFSET + offset, 0); offset++;
			/*device id lower 4 bits */
			data = this_key->ir_id & 0xF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;

			/* now write data 8 bits*/
			lpm_write8(lpm_drv, DATA_OFFSET + offset, ir_size);
			offset++;

			/*Copy time period now*/
			data = this_key->time_period & 0xFF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;
			data = (this_key->time_period >> 8) & 0xFF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;

			/*copy timeout now*/
			data = this_key->time_out & 0xFF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;
			data = (this_key->time_out >> 8) & 0xFF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;

			if (!this_key->tolerance)
				this_key->tolerance = 10;
			/*Copy tolerace*/
			data = this_key->tolerance ;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;
			/*Copy key idx*/
			data = key_info->key_index & 0xF;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data);
			offset++;

			/*Copy seq count*/
			data = key_info->num_patterns;
			lpm_write8(lpm_drv, DATA_OFFSET + offset, data) ;
			offset++;

			/* Now copy actual data*/
			offset = orginal_offset + 12;
			for (i = 0; i < key_info->num_patterns; i++) {
				key_info->fifo[i].mark /=
					this_key->time_period;
				data = key_info->fifo[i].mark;
				lpm_write8(lpm_drv,
					DATA_OFFSET + offset,
					data);
				offset++;
				key_info->fifo[i].symbol /=
					this_key->time_period;
				data = key_info->fifo[i].symbol;
				lpm_write8(lpm_drv, DATA_OFFSET + offset,
					data);
				offset++;
			}
		}
	/* We have finished writing to SBC memory */
	/* Send read command to SBC */
	/*msg format */
	/* byte 0 command id STM_LPM_MSG_BKBD_WRITE we have done with memory*/
	/* byte 1 trans id */
	/* byte 2 and byte 3 size of ir data we wrote*/
	/* byte 3,4,5,6 is offset at where we wrote the data */
	msg[0] = ir_size & BYTE_MASK;
	msg[1] = ir_size >> 8;
	msg[2] = orginal_offset & BYTE_MASK;
	msg[3] = orginal_offset >> 8 & BYTE_MASK;
	msg[4] = orginal_offset >> 16 & BYTE_MASK;
	msg[5] = orginal_offset>>24 & BYTE_MASK;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_BKBD_WRITE, msg, 6);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &resp);
	if (err < 0)
		break;
	/*we can not do much for this error just return to caller */
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_setup_ir);

/************************************************************************/
/* Purpose : To get wakeup data                                         */
/* This API will be called some kernel module                           */
/* This is to get application data                                      */
/* caller need to pass wakeup device and amount of data require         */
/* Return value would be valid size and data                            */
/* Host will use  following command                                     */
/* | byte0      |    byte1     | byte 2    | byte 3                     */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_GET_IRQ                                   */
/* byte2 and 3 size of data to read                                     */
/* There are two response from SBC possible                             */
/* if size of data is greate then 16 then SBC will give offset          */
/* else SBC will return data in mailbox it self                         */
/* SBC reply will be                                                    */
/* | byte0  |byte1                                                      */
/* Where byte0 is reply STM_LPM_MSG_GET_IRQ| STM_LPM_MSG_REPLY          */
/* byte 1 Transaction id                                                */
/* byte 2 and byte 3 is size,                                           */
/* byte 4 onwards is data                                               */
/* If this message has more bytes then message will be packed in SBC mem*/
/************************************************************************/
int stm_lpm_get_wakeup_info(enum stm_lpm_wakeup_devices *wakeupdevice,
				int *validsize, int datasize, char *data)
{
	int err = 0;
	unsigned int offset;
	u16 count;
	char msg[4] = {0};
	struct stlpm_internal_send_msg send_msg = {0};
	struct stm_lpm_message response = {0};
	msg[0] = *wakeupdevice;

	/* Copy size requested by application */
	msg[1] = datasize & BYTE_MASK;
	msg[2] = datasize >> 8 & BYTE_MASK;

	LPM_FILL_MSG(send_msg, STM_LPM_MSG_GET_IRQ, msg, 3);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_YES);
	err = lpm_exchange_msg(&send_msg, &response);
	if (err < 0)
		goto exit;

	/* Two response are possible*/
	if (response.command_id == STM_LPM_MSG_BKBD_READ) {
		/* if SBC replied by SBC memory */
		/* Get the offset to read from SBC memory */
		offset = response.msg_data[2] | response.msg_data[3] << 8;
		offset |= response.msg_data[3] << 16;
		offset |= response.msg_data[4] << 24;
		/* get valid size from SBC*/
		*validsize = lpm_read8(lpm_drv, DATA_OFFSET + offset + 2);
		*validsize |= lpm_read8(lpm_drv, DATA_OFFSET + offset + 3) << 8;
		/*Check if bit#15 is set in mailbox */
		if (*validsize & M_BIT_MASK)
			err = EAGAIN;
		/*reset 15 bit */
		*validsize &= M_BIT_MASK;
		/* Below condition is not possible */
		/*SBC have to provide data less than or equal to datasize */
		/* Added below check, if some bug pops up in firmware */
		if (*validsize > datasize)
				*validsize = datasize;

		for (count = 0; count < *validsize ; count++) {
			/*copy data to user */
			*data = lpm_read8(lpm_drv, DATA_OFFSET + offset + 4);
			data++;	offset++;
		}
	} else {
		*validsize = response.msg_data[0] | response.msg_data[1] << 8;
		/*Check if bit#15 is set in mailbox */
		if (*validsize & M_BIT_MASK)
			err = EAGAIN;
		/*reset 15 bit */
		*validsize &= M_BIT_MASK;
		/* Below condition is not possible */
		/*SBC have to provide data less than or equal to datasize */
		/* Added below check, if some bug pops up in firmware */
		if (*validsize > datasize)
			*validsize = datasize;
		/*copy data to user */
		memcpy(data, &response.msg_data[2], *validsize);

	}
exit:
	return err;
}
EXPORT_SYMBOL(stm_lpm_get_wakeup_info);

/************************************************************************/
/* Purpose : To get reset part of SOC                                   */
/* Host will use  following command                                     */
/* | byte0  |byte1      |byte2                                          */
/* | command id |    Transaction id |                                   */
/* | command id = STM_LPM_MSG_GEN_RESET                                 */
/* byte 2 is reset type required  as defined in enum stm_lpm_reset_type */
/* There will be no reply from SBC firmware                             */
/************************************************************************/

int stm_lpm_reset(enum stm_lpm_reset_type reset_type)
{
	int err = 0;
	struct stlpm_internal_send_msg send_msg = {0};
	struct stm_lpm_message response;
	char msg = reset_type;
	if (!reset_type)
		return -EINVAL;
	msg = reset_type;
	LPM_FILL_MSG(send_msg, STM_LPM_MSG_GEN_RESET, &msg, 1);
	LPM_FILL_ID_REPLY(send_msg, MSG_ID_AUTO, SBC_REPLY_NO);
	/* There will be error in case SBC firmware is not loaded*/
	err = lpm_exchange_msg(&send_msg, &response);
	if (err >= 0 && reset_type == STM_LPM_LPM_RESET) {
		/*Set the firmware as booting */
		int i = 0;
		mutex_lock(&lpm_drv->msg_protection_mutex);
		lpm_drv->sbc_state = STM_LPM_SBC_BOOT;
		mutex_unlock(&lpm_drv->msg_protection_mutex);
		/*wait till 1 second to get response from firmware */
		do {
			mdelay(100);
			err = stm_lpm_get_fw_state(&lpm_drv->sbc_state);
			if (err < 0 ||
				lpm_drv->sbc_state == STM_LPM_SBC_RUNNING)
				break;
			i++;
		} while (i != 10);
	}
	return err;
}
EXPORT_SYMBOL(stm_lpm_reset);


/************************************************************************/
/*                 Mailbox ISR                                          */
/* This will be executed whenever there is some request from SBC or     */
/* from message reply from SBC                                          */
/************************************************************************/

static irqreturn_t lpm_isr(int this_irq, void *params)
{
	struct stm_lpm_driver_data *lpm_drv_p;
	u32 msg_read[4], i;
	struct stm_lpm_message *msg;
	char *msg_p;
	lpm_drv_p = (struct stm_lpm_driver_data *)params;
	lpm_drv_p->reply_from_sbc = 0;

	/*Read the data from mailbox */
	/*SBC will always be in litte endian mode */
	/* if host is in big endian then reverse int */
	for (i = 0; i < 4; i++) {
		msg_read[i] = lpm_read32(lpm_drv_p, 1, MBX_READ_STATUS1 + i*4);
		if (is_bigendian())
			msg_read[i] = lpm_reverse_integer(msg_read[i]);
		}
	/*copy first message to check  this is reply from SBC or request*/
	msg_p = (char *) &msg_read[0];
	/*Check if reply from SBC and request from SBC */
	if ((*msg_p & STM_LPM_MSG_REPLY) ||
	   ((*msg_p & STM_LPM_MSG_BKBD_READ))) {
		msg = &lpm_drv_p->fw_reply_msg;
		lpm_drv_p->reply_from_sbc = 1;
	} else {
		msg = &lpm_drv_p->fw_request_msg;
	}
	/*Copy mailbox data into local structure*/
	memcpy(msg, &msg_read, 16);

	/*Clear mail box registery*/
	lpm_write32(lpm_drv_p, 1, MBX_READ_CLR_STATUS1, 0xFFFFFFFF);

	/* Signal work queue or API caller depending upon message from SBC */
	if (lpm_drv_p->reply_from_sbc == 1)
		wake_up_interruptible(&lpm_drv_p->stm_lpm_wait_queue);
	else
		schedule_work(&lpm_drv_p->lpm_sbc_reply_work);
	return IRQ_HANDLED;
}

/************************************************************************/
/*                 lpm_sbc_reply_worker                                 */
/* Whenever firmware request some information from Host                 */
/************************************************************************/
static void lpm_sbc_reply_worker(struct work_struct *work)
{
	struct stlpm_internal_send_msg send_msg = {0};
	unsigned char msg[5] = {0};
	struct stm_lpm_driver_data *lpm_drv_p;
	struct stm_lpm_message *msg_p;
	char msg_size, msg_id;
	lpm_drv_p = (struct stm_lpm_driver_data *)lpm_drv;
	msg_p = &lpm_drv_p->fw_request_msg;
	lpm_debug("Send reply to firmware\n");
	lpm_debug("recd command id %x\n", msg_p->command_id);
	if (msg_p->command_id == STM_LPM_MSG_VER) {
		/* In case firmware requested driver version*/
		msg[0] = STM_LPM_MAJOR_PROTO_VER << 4;
		msg[0] |= STM_LPM_MINOR_PROTO_VER;
		msg[1] = STM_LPM_MAJOR_SOFT_VER << 4;
		msg[1] |= STM_LPM_MINOR_SOFT_VER;
		msg[2] = STM_LPM_PATCH_SOFT_VER << 4;
		msg[2] |= STM_LPM_BUILD_MONTH;
		msg[3] = STM_LPM_BUILD_DAY;
		msg[4] = STM_LPM_BUILD_YEAR;
		msg_size = 5;
		msg_id = STM_LPM_MSG_VER | STM_LPM_MSG_REPLY;
	} else {
		/*send reply to SBC as error*/
		msg[0] = msg_p->command_id;
		msg[1] = -EINVAL;
		msg_size = 2;
		msg_id = STM_LPM_MSG_ERR;
	}
	LPM_FILL_MSG(send_msg, msg_id, msg, msg_size);
	LPM_FILL_ID_REPLY(send_msg, msg_p->transaction_id, SBC_REPLY_NO);
	lpm_exchange_msg(&send_msg, NULL);
	msg_p->command_id = 0;
}

/************************************************************************/
/*                 lpm_send_msg                                         */
/* Write message to mailbox                                             */
/* This message writing will generate interrupt for SBC CPU             */
/************************************************************************/

static int lpm_send_msg(struct stm_lpm_message *msg,
				unsigned char msg_size)
{
	int err = 0, count;
	u32 *tmp_i = (u32 *)msg;
	/*Check if firmware is loaded or not */
	if (!(lpm_drv->sbc_state == STM_LPM_SBC_RUNNING ||
		lpm_drv->sbc_state == STM_LPM_SBC_BOOT))
		return -EREMOTEIO;

	/* write data to mailbox, In case machine is big endian */
	/* reverse the data before writing */
	/*mailbox is 4 byte deep, we need to write 4 byte always*/
	for (count = (msg_size + 1)/4; count >= 0; count--) {
		if (is_bigendian())
			*(tmp_i + count) =
			lpm_reverse_integer(*(tmp_i + count));

			lpm_write32(lpm_drv, 1,	(MBX_WRITE_STATUS1 + (count*4)),
			*(tmp_i + count));
	}
	return err;
}

/************************************************************************/
/*                 lpm_get_response                                     */
/* to get SBC firmware response in no  irq mode                         */
/************************************************************************/
static int lpm_get_response(struct stm_lpm_message *response)
{
	int count;
	int msg_read1;
	/*Normally we expcet SBC reply in mailbox interrupt*/
	/*But when called in no irq mode, we have to poll to get response :( */
	/* Assuming poll time of 1 Second is good enough to see SBC reply */
	for (count = 0; count < 100; count++) {
		msg_read1 = lpm_read32(lpm_drv, 1, MBX_READ_STATUS1);
		/* If we received a reply then break the loop */
		if (msg_read1 & 0xFF) {
			if (is_bigendian())
				msg_read1 = lpm_reverse_integer(msg_read1);
		break;
		}
		mdelay(10);
	}

/* In case reply is not received in 1 second then firmware is not reponding*/
	if (count == 100) {
		printk(KERN_ERR "count %d value %x\n", count, msg_read1);
		return -EREMOTEIO;
	}
	/* Copy data received from mailbox*/
	memcpy(&lpm_drv->fw_reply_msg, (void *)&msg_read1, 4);
	lpm_write32(lpm_drv, 1, MBX_READ_CLR_STATUS1, 0xFFFFFFFF);

	/*return 1 to indicate that response is ok */
	return 1;
}
/************************************************************************/
/*                 lpm_exchange_msg                                     */
/* to send/received message with SBC CPU                                */
/************************************************************************/

int lpm_exchange_msg(struct stlpm_internal_send_msg *send_msg,
					struct stm_lpm_message *response)
{
	struct stm_lpm_message lpm_msg = {0};
	int count = 0, err = 0;
	memset(lpm_drv->fw_reply_msg.msg_data, 0x00, STM_LPM_MAX_MSG_DATA);
	lpm_debug("lpm_exchange_msg\n");
	/*Caller can expect reply, no reply or reply in poll mode */

	/*Lock the mailbox, prevent other caller to access MB write */
	if (send_msg->reply_type != SBC_REPLY_NO_IRQ)
		mutex_lock(&lpm_drv->msg_protection_mutex);
	/* in case API is called from no irq mode from Linux PM*/
	/* no more thread and isr are running no need to lock mutex here*/
	/*Also if mutex is not available then in case of no irq that will not be
	release therefore dead lock*/

	lpm_msg.command_id = send_msg->command_id;

	if (lpm_msg.command_id&STM_LPM_MSG_REPLY)
		lpm_msg.transaction_id = send_msg->trans_id;
	else
		lpm_msg.transaction_id = lpm_drv->glbtrans_id++;

	/*copy data into mailbox message */
	if (send_msg->msg_size)
		memcpy(&lpm_msg.msg_data, send_msg->msg, send_msg->msg_size);

	/*Print message information for debug purpose */
	lpm_debug("Sending msg {%x, %x ", lpm_msg.command_id,
				lpm_msg.transaction_id);
	for (count = 0; count < send_msg->msg_size; count++)
		lpm_debug(" %x", lpm_msg.msg_data[count]);
	lpm_debug(" }\n");

	lpm_drv->reply_from_sbc = 0;
	/*Send message to mailbox write */
	err = lpm_send_msg(&lpm_msg, send_msg->msg_size);
	/* if firmware is not loaded yet*/
	if (err < 0) {
		lpm_debug("firmware not loaded\n");
		goto exit_fun;
	}

	/*Depending upon reply type, wait for ISR or poll mailbox */
	switch (send_msg->reply_type) {
	case SBC_REPLY_NO_IRQ:
		err = lpm_get_response(response);
		break;
	case  SBC_REPLY_YES:
		/*wait for response here */
		err = wait_event_interruptible_timeout(
				lpm_drv->stm_lpm_wait_queue,
				lpm_drv->reply_from_sbc == 1,
				msecs_to_jiffies(100));
		break;
	case SBC_REPLY_NO:
		goto exit_fun;
		break;
	}
	/*debug purpose print received response from SBC */
	lpm_debug("recd reply %x {%x, %x ", err,
		lpm_drv->fw_reply_msg.command_id,
	lpm_drv->fw_reply_msg.transaction_id);
	for (count = 0; count < STM_LPM_MAX_MSG_DATA; count++)
		lpm_debug(" %x", lpm_drv->fw_reply_msg.msg_data[count]);
	lpm_debug("}\n");

	/* check wheather firmware reply is ok */
	if (err <= 0) {
		lpm_debug("f/w is not responding\n");
		err = -EAGAIN;
		goto exit_fun;
	}

	memcpy(response, &lpm_drv->fw_reply_msg,
		sizeof(struct stm_lpm_message));
	if (lpm_msg.transaction_id == lpm_drv->fw_reply_msg.transaction_id) {
		/* Check for error in FWLPM response*/
		if (response->command_id == STM_LPM_MSG_ERR) {
			/*just check error for command*/
			printk(KERN_ERR "Error respone\n");
			/* Get the actual error */
			/*Fix me : Actual error still to be decided by SBC */
			/* When available convert SBC error into kernel world */
			/* err = response->msg_data[3]; */
			/* convert err into linux work */
			/* till then EREMOTEIO is ok to indicate remote
			devide has not responded well  */
			err = -EREMOTEIO;
		}
	/* there is possibility we might get response for large msg*/
	} else if (response->command_id == STM_LPM_MSG_BKBD_READ) {
		lpm_debug("Got in reply a big message\n");
	} else {
		lpm_debug("Received ID %x\n", response->transaction_id);
	}

exit_fun:
	if (send_msg->reply_type != SBC_REPLY_NO_IRQ)
		mutex_unlock(&lpm_drv->msg_protection_mutex);
	return err;
}

/************************************************************************/
/*                 lpm_load_segment                                     */
/* load SBC firmware into SBC memory                                    */
/************************************************************************/
static int lpm_load_segment(struct stm_lpm_driver_data *lpm_drv_p,
				struct ELF64_info *elfinfo, int i)
{

	Elf64_Phdr *phdr = &elfinfo->progbase[i];
	void *data = elfinfo->base;
	signed long offset = DATA_OFFSET + phdr->p_paddr;
	unsigned long size = phdr->p_memsz;
	/*To check if we need to write onto PRG area or Data area*/
	/*generated elf by xp70 have offset 0x00400000 for program*/
	if (phdr->p_paddr == 0x00400000)
		offset = SBC_PRG_MEMORY_OFFSET;

	memcpy_toio((lpm_drv_p->lpm_mem_base[0] + offset),
		data + phdr->p_offset, size);
	return 0;
}

/************************************************************************/
/*                 lpm_load_fw                                          */
/* callback function of firmware load                                   */
/************************************************************************/
static int lpm_load_fw(const struct firmware *fw,
					struct stm_lpm_driver_data *lpm_drv_p)
{
	int err;
	struct ELF64_info *elfinfo = NULL;
	int i;

	if (!fw) {
		lpm_debug("LPM: Unable to load LPM firmware: not present?\n");
		return -EINVAL;
	}

	lpm_debug("LPM: Found sbc f/w\n");
	elfinfo = (struct ELF64_info *)ELF64_initFromMem((uint8_t *)fw->data,
				fw->size, 0);
	if (elfinfo == NULL)
			return -ENOMEM;

	for (i = 0; i < elfinfo->header->e_phnum; i++)
		if (elfinfo->progbase[i].p_type == PT_LOAD)
			lpm_load_segment(lpm_drv_p, elfinfo, i);

	/* Initialize sbc lpm */
	i = readl((u32)lpm_drv_p->lpm_mem_base[2] + SBC_CONFIG_OFFSET);
	i |= 0x1;
	writel(i, (u32)lpm_drv_p->lpm_mem_base[2] + SBC_CONFIG_OFFSET);
	i = 0;
	kfree((void *)elfinfo);

	/*Set the firmware as booting */
	mutex_lock(&lpm_drv->msg_protection_mutex);
	lpm_drv_p->sbc_state = STM_LPM_SBC_BOOT;
	mutex_unlock(&lpm_drv->msg_protection_mutex);
	/*wait till 1 second to get response from firmware */
	do {
		mdelay(100);
		err = stm_lpm_get_fw_state(&lpm_drv_p->sbc_state);
		if (err < 0 || lpm_drv_p->sbc_state == STM_LPM_SBC_RUNNING)
			break;
		i++;
	} while (i != 10);
	{
		struct stm_lpm_version driver_ver, fw_ver;
		err = stm_lpm_get_version(&driver_ver, &fw_ver);
		if (err >= 0)
		{	
			printk("Firmware version %d.%d%d \n",
			fw_ver.major_soft,
			fw_ver.minor_soft,
			fw_ver.patch_soft);
			
		}
	}
	if (lpm_drv_p->plat_data) {
		struct stm_lpm_pio_setting pio_setting = {
			.pio_bank = stm_gpio_port(lpm_drv_p->plat_data->gpio),
			.pio_pin = stm_gpio_pin(lpm_drv_p->plat_data->gpio),
			.pio_level = (lpm_drv_p->plat_data->trigger_level
					? STM_LPM_PIO_LOW
					: STM_LPM_PIO_HIGH),
			.pio_use = STM_LPM_PIO_POWER,
			.pio_direction = STM_LPM_PIO_OUTPUT,
			.interrupt_enabled = 0,
		};

		err = stm_lpm_setup_pio(&pio_setting);
		if (err < 0)
			pr_err("stm_lpm_setup_pio failed: %d\n" , err);
	}

	/*in load function do not return error if caused by */
	return 1;
}
/************************************************************************/
/*                 lpm_load_firmware                                    */
/* register callback to get firmware                                    */
/************************************************************************/

static int lpm_load_firmware(struct platform_device *pdev)
{
	int err;
	int result;
	struct stm_lpm_driver_data *lpm_drv_p;
	enum stm_lpm_sbc_state fw_state;
	lpm_drv_p = platform_get_drvdata(pdev);
	result = snprintf(lpm_drv_p->fw_name, sizeof(lpm_drv_p->fw_name),
			"lpm_fw%s.elf", lpm_get_cpu_type());

	/* was the string truncated? */
	BUG_ON(result >= sizeof(lpm_drv_p->fw_name));

	/*Set the firmware as booting */
	mutex_lock(&lpm_drv->msg_protection_mutex);
	lpm_drv_p->sbc_state = STM_LPM_SBC_BOOT;
	mutex_unlock(&lpm_drv->msg_protection_mutex);

	if(stm_lpm_get_fw_state(&fw_state) < 0)
		lpm_debug(" %s firmware not loaded\n",__func__);
	else {
		lpm_debug("firmware already loaded\n");
		return 0;
	}
	lpm_debug("LPM: Loading Firmware (%s)...\n",
		lpm_drv_p->fw_name);

	err = request_firmware_nowait(THIS_MODULE, 1, lpm_drv_p->fw_name,
		&pdev->dev, GFP_KERNEL,
		(struct stm_lpm_driver_data *)lpm_drv_p , (void *)lpm_load_fw);
	if (err)
		return -ENOMEM;

	return 0;
}

/*
 * enum stm_lpm_wakeup_devices {
 *         STM_LPM_WAKEUP_IR  = 1<<0,
 *         STM_LPM_WAKEUP_CEC = 1<<1,
 *         STM_LPM_WAKEUP_FRP = 1<<2,
 *         STM_LPM_WAKEUP_WOL = 1<<3,
 *         STM_LPM_WAKEUP_RTC = 1<<4,
 *         STM_LPM_WAKEUP_ASC = 1<<5,
 *         STM_LPM_WAKEUP_NMI = 1<<6,
 *         STM_LPM_WAKEUP_HPD = 1<<7,
 *         STM_LPM_WAKEUP_PIO = 1<<8,
 *         STM_LPM_WAKEUP_EXT = 1<<9
 * };
 * */
static ssize_t stm_lpm_show_wakeup(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	char data[2] = {0, 0};
	enum stm_lpm_wakeup_devices wakeup_device = 0;
	if (stm_lpm_get_wakeup_device(&wakeup_device) < 0)
		lpm_debug("<%s> firmware not responding\n", __func__);
	if (STM_LPM_WAKEUP_PIO & wakeup_device)
		if (stm_lpm_get_trigger_data(8, 4, 4, data) < 0)
			lpm_debug("<%s> trigger data message failed\n", __func__);

	return sprintf(buf, "%x %x %x\n", wakeup_device, data[0], data[1]);
}

static DEVICE_ATTR(wakeup, S_IRUGO | S_IWUSR, stm_lpm_show_wakeup,
		NULL);

/************************************************************************/
/*                 stm_sbc_probe                                        */
/************************************************************************/
static int __devinit stm_sbc_probe(struct platform_device *pdev)
{
	lpm_debug("%s\n", __func__);
	lpm_drv->plat_data = dev_get_platdata(&pdev->dev);
	return 0;
}

static int __devexit stm_sbc_remove(struct platform_device *pdev)
{
	lpm_drv->plat_data = NULL;
	return 0;
}

/************************************************************************/
/*                 stm_lpm_probe                                        */
/************************************************************************/
static int __devinit stm_lpm_probe(struct platform_device *pdev)
{
	struct resource *res;
	int err = 0;
	lpm_debug("%s\n", __func__);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	lpm_debug("mem:SBC res->start %x %x\n", res->start, res->end);
	if (!devm_request_mem_region(&pdev->dev, res->start,
		res->end - res->start, "stm-lpm")) {

		printk(KERN_ERR "%s: Request mem 0x%x region not done\n",
				__func__, res->start);
		return -ENOMEM;
	}

	lpm_drv = kzalloc(sizeof(struct stm_lpm_driver_data), GFP_KERNEL);
	if (unlikely(lpm_drv == NULL)) {
		printk(KERN_ERR "%s: Request memory not done\n", __func__);
		return -ENOMEM;
	}

	lpm_drv->lpm_mem_base[0] = devm_ioremap_nocache(&pdev->dev,
			res->start, (int)(res->end - res->start));
	if (!lpm_drv->lpm_mem_base[0]) {
		printk(KERN_ERR "%s: Request iomem 0x%x region not done\n",
			__func__, (unsigned int)res->start);
		err = -ENOMEM;
		goto free_and_exit;
	}
	lpm_debug("lpm_add %x\n", (unsigned int)lpm_drv->lpm_mem_base[0]);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;
	lpm_debug("mem:MB res->start %x %x\n", res->start, res->end);
	if (!devm_request_mem_region(&pdev->dev, res->start,
		res->end - res->start, "stm-lpm")) {

		printk(KERN_ERR "%s: Request mem:MB 0x%x region not done\n",
				__func__, res->start);
		return -ENOMEM;
	}

	lpm_drv->lpm_mem_base[1] = devm_ioremap_nocache(&pdev->dev,
			res->start, (int)(res->end - res->start));
	if (!lpm_drv->lpm_mem_base[1]) {
		printk(KERN_ERR "%s: Request iomem 0x%x region not done\n",
			__func__, (unsigned int)res->start);
		err = -ENOMEM;
		goto free_and_exit;
	}
	lpm_debug("lpm_add MB %x\n", (unsigned int)lpm_drv->lpm_mem_base[1]);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res)
		return -ENODEV;
	lpm_debug("mem:CONF res->start %x %x\n", res->start, res->end);
	if (!devm_request_mem_region(&pdev->dev, res->start,
		res->end - res->start, "stm-lpm")) {

		printk(KERN_ERR "%s: Request mem:CONF 0x%x region not done\n",
				__func__, res->start);
		return -ENOMEM;
	}

	lpm_drv->lpm_mem_base[2] = devm_ioremap_nocache(&pdev->dev,
			res->start, (int)(res->end - res->start));
	if (!lpm_drv->lpm_mem_base[2]) {
		printk(KERN_ERR "%s: Request iomem 0x%x region not done\n",
			__func__, (unsigned int)res->start);
		err = -ENOMEM;
		goto free_and_exit;
	}
	lpm_debug("lpm CONF %x\n", (unsigned int)lpm_drv->lpm_mem_base[2]);
	/*irq request */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_ERR "%s Request irq %x not done\n",
			__func__, res->start);
		err = -ENODEV;
		goto free_and_exit;
	}
	/*Semaphore initialization*/
	init_waitqueue_head(&lpm_drv->stm_lpm_wait_queue);
	mutex_init(&lpm_drv->msg_protection_mutex);

	/*
	* Work struct init
	* lpm does not need dedicate work queue
	* use default queue
	*/
	INIT_WORK(&lpm_drv->lpm_sbc_reply_work, lpm_sbc_reply_worker);
	if (devm_request_irq(&pdev->dev, res->start, lpm_isr,
		IRQF_DISABLED, "stlmp", (void *)lpm_drv) < 0) {
		printk(KERN_ERR "%s: Request stlpm irq not done\n",
			__func__);
		err = -ENODEV;
		goto free_and_exit;
	}
	/*set driver specific data */
	platform_set_drvdata(pdev, lpm_drv);

	/*
	 * Program Mailbox for interrupt enable
	 */
	if (is_bigendian())
		lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF000000);
	else
		lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF);

	lpm_write32(lpm_drv, 1, MBX_WRITE_STATUS1, 0);

	lpm_load_firmware(pdev);

	err = device_create_file(&pdev->dev, &dev_attr_wakeup);

	if (err) {
		dev_err(&pdev->dev, "Cannot create wakeup sysfs entry\n");
		return err;
	}

	err = platform_driver_register(&stm_sbc_driver);
	if (err)
		pr_err("STM_SBC driver registration failed: %d\n" , err);
	else
		pr_info("STM_SBC driver registered\n");

	return err;

free_and_exit:
	kfree(lpm_drv);
	return err;
}

static int __devexit stm_lpm_remove(struct platform_device *pdev)
{
	struct stm_lpm_driver_data *lpm_drv_p;
	lpm_debug("stm_lpm_remove\n");
	lpm_drv_p = platform_get_drvdata(pdev);
	kfree(lpm_drv_p);
	return 0;
}

/*freeze and suspend are not really required */
static int stm_lpm_freeze(struct device *dev)
{
	lpm_debug("stm_lpm_freeze\n");
	return 0;
}

static int stm_lpm_restore(struct device *dev)
{
	lpm_debug("stm_lpm_restore\n");
	/*WA to enable mailbox interrupt on CPS exit */
	/*MB interrupt are getting disable , reason not understood */
	/*Enable again here*/
	if (is_bigendian())
		lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF000000);
	else
		lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF);
	return 0;
}

static const struct dev_pm_ops stm_lpm_pm_ops = {
		.freeze = stm_lpm_freeze,
		.restore = stm_lpm_restore,
};

static struct platform_driver stm_lpm_driver = {
	.driver.name = "stm-lpm",
	.driver.owner = THIS_MODULE,
	.driver.pm = &stm_lpm_pm_ops,
	.probe = stm_lpm_probe,
	.remove = __devexit_p(stm_lpm_remove),
};

static struct platform_driver stm_sbc_driver = {
	.driver.name = "stm-sbc",
	.driver.owner = THIS_MODULE,
	.probe = stm_sbc_probe,
	.remove = __devexit_p(stm_sbc_remove),
};

static int __init stm_lpm_init(void)
{
	int err = 0;

	err = platform_driver_register(&stm_lpm_driver);
	if (err)
		pr_err("STM_LPM driver registration failed: %d\n" , err);
	else
		pr_info("STM_LPM driver registered\n");
	return err;
}

void __exit stm_lpm_exit(void)
{
	printk(KERN_ERR "STM_LPM driver removed\n");
	platform_driver_unregister(&stm_lpm_driver);
}

module_init(stm_lpm_init);
module_exit(stm_lpm_exit);

MODULE_AUTHOR("STMicroelectronics  <www.st.com>");
MODULE_DESCRIPTION("lpm device driver for STMicroelectronics devices");
MODULE_LICENSE("GPL");

/* ---------------------------------------------------------------------------
 * Copyright (C) 2012 STMicroelectronics Limited
 *
 * Author: Pooja Agarwal <pooja.agarwal@st.com>
 * Author: Udit Kumar <udit-dlh.kumar@st.cm>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 * ----------------------------------------------------------------------------
 */
#ifndef __STM_LPM_DEF_H_
#define __STM_LPM_DEF_H_

/* LPM protocol has following architecture				*/
/* | byte0	|byte1	|	byte2	|	byte3	|		*/
/* | command id |	Transaction id |	msg data|	msg data|*/
/* msg data size can vary depending upon what is command		*/

/* In case of internal SBC, communcation will be done using mailbox */
/* When this is not possible to fill message in mailbox of 16 bytes */
/*Such message will be send by directly writing into DMEM of SBC	*/

/* In case of external SBC (7108), which is connected with SOC via i2c*/
/* Communcation will be done using i2c line */

/* In both cases other CPU expect message to be in little endian mode*/

/*
 * Message commands id
 */
 /* No opertion */
#define STM_LPM_MSG_NOP           0x0

/* command id to retrive version number */
#define STM_LPM_MSG_VER           0x1

/*Command id to read current RTC value,*/
/* When SBC wants to know current RTC time*/
#define STM_LPM_MSG_READ_RTC      0x3

/*command id to trim RTC*/
#define STM_LPM_MSG_SET_TRIM      0x4

/* command id to enter in passive standny mode*/
#define STM_LPM_MSG_ENTER_PASSIVE 0x5

/* command id  to set watch dog timeout of SBC*/
#define STM_LPM_MSG_SET_WDT       0x6

/* command id  to set new RTC value for SBC*/
#define STM_LPM_MSG_SET_RTC       0x7

/* command id  to configure frontpanel LED ownership
and display mode*/
#define STM_LPM_MSG_SET_FP        0x8

/*command id to set wakeup time*/
#define STM_LPM_MSG_SET_TIMER     0x9

/* command id to get status of SBC CPU */
#define STM_LPM_MSG_GET_STATUS    0xA

/* command id to generate reset  */
#define STM_LPM_MSG_GEN_RESET     0xB

/* command id to set wakeup device  */
#define STM_LPM_MSG_SET_WUD       0xC

/* command id to get wakeup device  */
#define STM_LPM_MSG_GET_WUD       0xD

/* command id to offset in SBC memory  */
#define STM_LPM_MSG_LGWR_OFFSET   0x10

#define STM_LPM_MSG_SET_PIO   0x11
#define STM_LPM_MSG_GET_ADV_FEA 0x12
#define STM_LPM_MSG_SET_ADV_FEA 0x13
#define STM_LPM_MSG_SET_KEY_SCAN 0x14

/* command id to set IR information for SBC */
/* IR keys on which we should do wakeup */
#define STM_LPM_MSG_SET_IR        0x41

/* command id to get data associated with some interrupt */
#define STM_LPM_MSG_GET_IRQ       0x42

/* command id to inform trace data to host  */
/* SBC can send trace data to host using this command */
#define STM_LPM_MSG_TRACE_DATA    0x43

/* command id to read message from SBC memory */
#define STM_LPM_MSG_BKBD_READ     0x44

/* command id inform SBC that write to SBC memory is done */
#define STM_LPM_MSG_BKBD_WRITE    0x45

/* Indicate bit-7 of command id as reply  */
#define STM_LPM_MSG_REPLY	  0x80

/* Command for error   */
#define STM_LPM_MSG_ERR           0x82

/* Version number of driver , This has following fields */
/* Protocol major and minor number */
/* Software major, minor and patch number */
/* Software release build, month, day and year */
#define STM_LPM_MAJOR_PROTO_VER      3
#define STM_LPM_MINOR_PROTO_VER      0
#define STM_LPM_MAJOR_SOFT_VER       1
#define STM_LPM_MINOR_SOFT_VER       2
#define STM_LPM_PATCH_SOFT_VER       0
#define STM_LPM_BUILD_MONTH         03
#define STM_LPM_BUILD_DAY           21
#define STM_LPM_BUILD_YEAR          12


/* The maximum size of message data that can be send over mailbox */
/* Mailbox is 16 byte deep, and 2 bytes are reserved for command
and transaction id */
#define STM_LPM_MAX_MSG_DATA 14

/* This is address of i2c bus connected with 7108 FP */
/* This is coded as in stm8 firmware */
/*i2c communication is used in case of external SBC */
#define ADDRESS_OF_EXT_MC 0x94

/*
 * For actual H205/H415
 */
/* Various offset of LPM IP, These offsets are w.r.t LPM base address */
/* These are valid for SOC having internal SBC */

/* SBC data memory offset as seen by Host*/
#define DATA_OFFSET 0x010000

/* SBC program memory as offset seen by Host*/
#define SBC_PRG_MEMORY_OFFSET 0x018000

/* SBC mailbox offset as seen by Host*/
/* #define SBC_MBX_OFFSET 0xB4000 */
#define SBC_MBX_OFFSET 0

/*SBC configuration register offset as seen on Host*/
/*#define SBC_CONFIG_OFFSET 0xB5100*/
#define SBC_CONFIG_OFFSET 0

/* Mailbox registers to be written by Host */
/*SBC firmware will read below registers to get host message*/
/* There are four such registers in mailbox each of 4 bytes */
#define MBX_WRITE_STATUS1  (SBC_MBX_OFFSET + 0x004)
#define MBX_WRITE_STATUS2  (SBC_MBX_OFFSET + 0x008)
#define MBX_WRITE_STATUS3  (SBC_MBX_OFFSET + 0x00C)
#define MBX_WRITE_STATUS4  (SBC_MBX_OFFSET + 0x010)

/* Mailbox registers to be read by Host */
/*SBC firmware will write below registers to send message */
/* There are four such registers in mailbox each of 4 bytes */
#define MBX_READ_STATUS1  (SBC_MBX_OFFSET + 0x104)
#define MBX_READ_STATUS2  (SBC_MBX_OFFSET + 0x108)
#define MBX_READ_STATUS3  (SBC_MBX_OFFSET + 0x10C)
#define MBX_READ_STATUS4  (SBC_MBX_OFFSET + 0x110)

/* To clear mailbox interrupt status */
#define MBX_READ_CLR_STATUS1  (SBC_MBX_OFFSET + 0x144)

/* To enable/disable mailbox interrupt on Host :RW */
#define MBX_INT_ENABLE (SBC_MBX_OFFSET + 0x164)

/* To enable mailbox interrupt on Host : WO only set allowed */
#define MBX_INT_SET_ENABLE (SBC_MBX_OFFSET + 0x184)

/* To disable mailbox interrupt on Host : WO only clear allowed */
#define MBX_INT_CLR_ENABLE (SBC_MBX_OFFSET + 0x1A4)

/* From host there are three type of message can be send to SBC*/

/* No reply expected from SBC i.e reset SBC, Passive standby */
#define SBC_REPLY_NO		0

/* Reply is expected from SBC i.e. get version etc and reply
is expected by mailbox interrupts */
#define SBC_REPLY_YES		0x1

/* Message is send to SBC when irq are disabled, in such case */
/* We have to poll mailbox to get response from SBC*/
#define SBC_REPLY_NO_IRQ	0x2

/* Used to mask a byte */
#define BYTE_MASK		0xFF

/* For FP setting, mask to get owner's 2 bits  */
#define OWNER_MASK		0x3

/* For FP setting, mask to brightness of LED 's 4 bits  */
#define BRIGHT_MASK		0xF

/* mask to get 15th bit from SBC response */
/* If this is set then more data is available in SBC */
#define M_BIT_MASK		0x7F

/*Message send to SBC does not have msg data  */
#define MSG_ZERO_SIZE		0

/* Transaction id will be generated by lpm itself */
#define MSG_ID_AUTO		0

/* to write 8 bit data into LPM */
#define lpm_write8(drv, offset, value)     iowrite8(value, \
				((u32)drv->lpm_mem_base[0]) + (offset))

/* to read 8 bit data into LPM */
#define lpm_read8(drv, offset)   ioread8(((u32)drv->lpm_mem_base[0]) + (offset))

/* to write 32 bit data into LPM */
#define lpm_write32(drv, index, offset, value)    iowrite32(value, \
			(u32)(drv->lpm_mem_base[index]) + (offset))

/* to read 32 bit data into LPM */
#define lpm_read32(drv, id, offset)	ioread32((u32)(drv->lpm_mem_base[id]) \
					+ offset)

/* Below is message struct to passed to other CPU */
/* Normally each message is less than 16 bytes */

/* In case of internal SBC,
If message is large then 16 bytes then direct write to SBC memory is done*/

/* In case of external SBC,
If message is large then 16 bytes then such cases are handled separately */

struct stm_lpm_message {
	unsigned char command_id;
	unsigned char transaction_id;
	unsigned char msg_data[STM_LPM_MAX_MSG_DATA];
} __packed;

/* This is internal struct */
/* This is used to fill message, control of reply and message size*/
struct stlpm_internal_send_msg {
	unsigned char command_id;
	unsigned char *msg;
	unsigned char msg_size;
	unsigned char trans_id;
	unsigned char reply_type;
};


/* This function is use to exchange message with SBC */
/* First field is for message to send and second is for response */
/* Based upon message reply type, response field can be ignored*/
int lpm_exchange_msg(struct stlpm_internal_send_msg *send_msg,
	struct stm_lpm_message *response);

/* Macro to fill stlpm_internal_send_msg struct field */
#define LPM_FILL_MSG(lpm_msg, cmd_id, msg_bug, size) { \
	lpm_msg.command_id = cmd_id;			\
	lpm_msg.msg = msg_bug;				\
	lpm_msg.msg_size = size;			\
}

#define LPM_FILL_ID_REPLY(lpm_msg, id, reply) { \
	lpm_msg.trans_id = id;				\
	lpm_msg.reply_type = reply; }

/* to get cpu type */
const char *lpm_get_cpu_type(void);

#ifdef CONFIG_STM_LPM_RD_MONITOR
/* to monitor power pin on 7108*/
void stm_lpm_stop_power_monitor(struct i2c_client *client);
int  stm_lpm_start_power_monitor(struct i2c_client *client);
#endif

/*inline function reverse integer on BE machine */
static inline int lpm_reverse_integer(int i)
{
	unsigned char c1, c2, c3, c4;
	c1 = i & 255;
	c2 = (i >> 8) & 255;
	c3 = (i >> 16) & 255;
	c4 = (i >> 24) & 255;
	return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}

/*inline function to check endianess of machine*/
static inline int is_bigendian(void)
{
	int i = 1;
	char *p = (char *)&i;
	if (*p == 0)
		return 1;
	else
		return 0;
}

#endif /*__LPM_DEF_H*/

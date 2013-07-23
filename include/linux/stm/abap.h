
/*
 * (c) 2012 STMicroelectronics Limited
 *
 * Author: Srinivas Kandagatla <srinivas.kandagatla@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_STM_ABAP_H
#define __LINUX_STM_ABAP_H

/*
 * ABAP has 8 programmable registers from ABAP_PRG_BASE
 * these registers can be used to put some code to park
 * secondary cores during kexec case.
 */
#define ABAP_PRG_BASE		0xffff0020

#define ABAP_JUMP_ADDR		(0x1c)

extern const unsigned int stm_abap_sec_start_size;
extern void stm_abap_sec_start(void);
#endif

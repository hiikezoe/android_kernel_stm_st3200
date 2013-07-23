/*
 *  linux/arch/arm/mach-stm/headsmp.S
 *
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>

#ifdef CONFIG_KEXEC
ENTRY(stm_abap_sec_start)
1:
	wfe
	ldr	r5, [pc, #16]
	teq	r5, #0x0
	beq	1b
	str	pc, [pc, #4]
	mov	pc, r5
ENDPROC(stm_abap_sec_start)
stm_abap_sec_start_end:

	.globl stm_abap_sec_start_size
stm_abap_sec_start_size:
	.long stm_abap_sec_start_end - stm_abap_sec_start
#endif

	__INIT

/*
 * STM specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 */
ENTRY(stm_secondary_startup)
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, r0, #15
	adr	r4, 1f
	ldmia	r4, {r5, r6}
	sub	r4, r4, r5
	add	r6, r6, r4
pen:	ldr	r7, [r6]
	cmp	r7, r0
	bne	pen

	/*
	 * we've been released from the holding pen: secondary_stack
	 * should now contain the SVC stack for this core
	 */
	b	secondary_startup

1:	.long	.
	.long	pen_release

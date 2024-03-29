/*
 *  KPTrace - Kprobes-based tracing
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) STMicroelectronics, 2008, 2012
 *
 * 2007-Jul	Created by Chris Smith <chris.smith@st.com>
 * 2008-Aug     Chris Smith <chris.smith@st.com> added a sysfs interface for
 *              user space tracing.
 */
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/prctl.h>
#include <linux/relay.h>
#include <linux/debugfs.h>
#include <linux/futex.h>
#include <linux/version.h>
#include <trace/kptrace.h>
#include <net/sock.h>
#include <asm/sections.h>

#include <asm/kptrace_target.h>

/*
 * target specific context switch handler
 * */
static int context_switch_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	char tbuf[KPTRACE_SMALL_BUF];
	int prev, now;
	now = current->pid;
	prev = ((struct task_struct *)regs->ARG1)->pid;

	snprintf(tbuf, KPTRACE_SMALL_BUF, "C %d %d", prev, now);
	kptrace_write_trace_record(p, regs, tbuf);
	return 0;
}

#ifdef CONFIG_SMP
static int ipi_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	char tbuf[KPTRACE_SMALL_BUF];
	snprintf(tbuf, KPTRACE_SMALL_BUF, "IP %.8x", (int)regs->PC);

	kptrace_write_trace_record(p, regs, tbuf);
	return 0;
}

static int ipi_rp_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	kptrace_write_trace_record_no_callstack("ip");
	return 0;
}
#endif

/*
 * target specific core events
 * */
void init_core_event_logging(struct kp_tracepoint_set *set)
{
	/* get context switches before finish_task_switch on ARM */
	kptrace_create_tracepoint(set, "finish_task_switch",
			context_switch_pre_handler, NULL);

	kptrace_create_tracepoint(set, "gt_clockevent_interrupt",
			irq_pre_handler, irq_rp_handler);

#ifdef CONFIG_SMP
	kptrace_create_tracepoint(set, "twd_handler", irq_pre_handler,
					irq_rp_handler);
	kptrace_create_tracepoint(set, "scheduler_ipi", ipi_pre_handler,
					ipi_rp_handler);
	kptrace_create_tracepoint(set, "generic_smp_call_function_interrupt",
					ipi_pre_handler, ipi_rp_handler);
	kptrace_create_tracepoint(set,
			"generic_smp_call_function_single_interrupt",
			ipi_pre_handler, ipi_rp_handler);
#endif
}

 /*
  * target specific syscalls
  * */
void init_syscall_logging(struct kp_tracepoint_set *set)
{
	/*CALL(sys_sigaltstack) TODO */

	CALL_ABI(sys_pread64, sys_oabi_pread64)
	CALL_ABI(sys_pwrite64, sys_oabi_pwrite64)
	CALL_ABI(sys_truncate64, sys_oabi_truncate64)
	CALL_ABI(sys_ftruncate64, sys_oabi_ftruncate64)
	CALL_ABI(sys_stat64, sys_oabi_stat64)
	CALL_ABI(sys_lstat64, sys_oabi_lstat64)
	CALL_ABI(sys_fstat64, sys_oabi_fstat64)
	CALL_CUSTOM_PRE_ABI(sys_fcntl64, sys_oabi_fcntl64,
				 syscall_ihhh_pre_handler);
	CALL_ABI(sys_readahead, sys_oabi_readahead)
	CALL_ABI(sys_epoll_ctl, sys_oabi_epoll_ctl)
	CALL_ABI(sys_epoll_wait, sys_oabi_epoll_wait)
	CALL(sys_pciconfig_iobase)
	CALL(sys_pciconfig_read)
	CALL(sys_pciconfig_write)
	CALL_ABI(sys_bind, sys_oabi_bind)
	CALL_ABI(sys_connect, sys_oabi_connect)
	CALL_ABI(sys_sendto, sys_oabi_sendto)
	CALL_ABI(sys_sendmsg, sys_oabi_sendmsg)
	CALL_ABI(sys_semop, sys_oabi_semop)
	CALL_ABI(sys_semtimedop, sys_oabi_semtimedop)
	CALL_ABI(sys_fstatat64,  sys_oabi_fstatat64)
	CALL(sys_sync_file_range2)
	CALL(sys_utimensat)
	CALL(sys_mremap)
	CALL(sys_signalfd)
	CALL(sys_timerfd_create)
	CALL(sys_eventfd)
	CALL(sys_fallocate)
	CALL(sys_timerfd_settime)
	CALL(sys_timerfd_gettime)
	CALL(sys_signalfd4)
	CALL(sys_eventfd2)
	CALL(sys_epoll_create1)
	CALL_CUSTOM_PRE(sys_dup3, syscall_iihh_pre_handler);
	CALL(sys_pipe2)
	CALL(sys_inotify_init1)
	CALL(sys_getcpu)

#undef ABI
}

void init_memory_logging(struct kp_tracepoint_set *set)
{
	kptrace_create_tracepoint(set, "__alloc_pages_nodemask",
			alloc_pages_pre_handler, alloc_pages_rp_handler);
}

/**/
struct kp_target_t kp_target_arm = {
	.init_core_event_logging = init_core_event_logging,
	.init_syscall_logging = init_syscall_logging,
	.init_memory_logging = init_memory_logging
};

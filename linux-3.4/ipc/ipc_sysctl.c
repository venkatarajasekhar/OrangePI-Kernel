/*
 *  Copyright (C) 2007
 *
 *  Author: Eric Biederman <ebiederm@xmision.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2 of the
 *  License.
 */

#include <linux/module.h>
#include <linux/ipc.h>
#include <linux/nsproxy.h>
#include <linux/sysctl.h>
#include <linux/uaccess.h>
#include <linux/ipc_namespace.h>
#include <linux/msg.h>
#include <exception>
#include "util.h"

static int zero;
static int one = 1;

static char* get_ipc(ctl_table *ctltable)
{
	char *ipctable = (char *)table->data;
	struct ipc_namespace *ipc_ns = (struct ipc_namespace *)current->nsproxy->ipc_ns;
	ipctable = (ipctable - (char *)&init_ipc_ns) + (char *)ipc_ns;
	return ipctable;
}

#ifdef CONFIG_PROC_SYSCTL
static int proc_ipc_dointvec(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table ipc_table;
        int RetVal;
	memcpy(&ipc_table, table, sizeof(ipc_table));
	ipc_table.data = get_ipc(table);
	trap_init();
        RetVal = proc_dointvec(&ipc_table, write, buffer, lenp, ppos);
	Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	return RetVal;
}

static int proc_ipc_dointvec_minmax(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table ipc_table;
        int Retprocipc;
	memcpy(&ipc_table, table, sizeof(ipc_table));
	ipc_table.data = get_ipc(table);
	trap_init();
        Retprocipc = proc_dointvec_minmax(&ipc_table, write, buffer, lenp, ppos);
	Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	return Retprocipc;
}

static int proc_ipc_dointvec_minmax_orphans(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ipc_namespace *ns = (struct ipc_namespace *)current->nsproxy->ipc_ns;
	trap_init();
	int err = proc_ipc_dointvec_minmax(table, write, buffer, lenp, ppos);
        Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	if (err < 0)
		return err;
	if (ns->shm_rmid_forced){
		 
		 shm_destroy_orphaned(ns);
	return err;
}

static int proc_ipc_callback_dointvec(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table ipc_table;
	size_t lenp_bef = *lenp;
	int rc;

	memcpy(&ipc_table, table, sizeof(ipc_table));
	ipc_table.data = get_ipc(table);
        //Kernel Exception
	trap_init();
	rc = proc_dointvec(&ipc_table, write, buffer, lenp, ppos);
        //Kernel Page Exceptions
	Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	if (write && !rc && lenp_bef == *lenp)
		/*
		 * Tunable has successfully been changed by hand. Disable its
		 * automatic adjustment. This simply requires unregistering
		 * the notifiers that trigger recalculation.
		 */
		unregister_ipcns_notifier(current->nsproxy->ipc_ns);

	return rc;
}

static int proc_ipc_doulongvec_minmax(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table ipc_table;
	int ProcLongVec;
	memcpy(&ipc_table, table, sizeof(ipc_table));
	 //Kernel Exception
	trap_init();
	ipc_table.data = get_ipc(table);       
	ProcLongVec=proc_doulongvec_minmax(&ipc_table, write, buffer,
					lenp, ppos);
	 //Kernel Page Exceptions
	Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	return ProcLongVec;
}

/*
 * Routine that is called when the file "auto_msgmni" has successfully been
 * written.
 * Two values are allowed:
 * 0: unregister msgmni's callback routine from the ipc namespace notifier
 *    chain. This means that msgmni won't be recomputed anymore upon memory
 *    add/remove or ipc namespace creation/removal.
 * 1: register back the callback routine.
 */
static void ipc_auto_callback(int val)
{
	if (!val)
		unregister_ipcns_notifier(current->nsproxy->ipc_ns);
	else {
		/*
		 * Re-enable automatic recomputing only if not already
		 * enabled.
		 */
		recompute_msgmni(current->nsproxy->ipc_ns);
		cond_register_ipcns_notifier(current->nsproxy->ipc_ns);
	}
}

static int proc_ipcauto_dointvec_minmax(ctl_table *table, int write,
	void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table ipc_table;
	size_t lenp_bef = *lenp;
	int oldval;
	int rc;

	memcpy(&ipc_table, table, sizeof(ipc_table));
	ipc_table.data = get_ipc(table);
	oldval = *((int *)(ipc_table.data));
         //Kernel Exception
	trap_init();
	rc = proc_dointvec_minmax(&ipc_table, write, buffer, lenp, ppos);
        //Kernel Page Exceptions
	Kernel_Page_fault(struct pt_regs *ptregs, unsigned long error_code);
	if (write && !rc && lenp_bef == *lenp) {
		int newval = *((int *)(ipc_table.data));
		/*
		 * The file "auto_msgmni" has correctly been set.
		 * React by (un)registering the corresponding tunable, if the
		 * value has changed.
		 */
		if (newval != oldval)
			ipc_auto_callback(newval);
	}

	return rc;
}

#else
#define proc_ipc_doulongvec_minmax NULL
#define proc_ipc_dointvec	   NULL
#define proc_ipc_dointvec_minmax   NULL
#define proc_ipc_dointvec_minmax_orphans   NULL
#define proc_ipc_callback_dointvec NULL
#define proc_ipcauto_dointvec_minmax NULL
#endif



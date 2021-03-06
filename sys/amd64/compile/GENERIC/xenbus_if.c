/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../xen/xenbus/xenbus_if.m
 * with
 *   makeobjops.awk
 *
 * See the source file for legal information
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/bus.h>
#include <machine/atomic.h>
#include <xen/xen-os.h>
#include <xen/evtchn.h>
#include <xen/xenbus/xenbusvar.h>
#include "xenbus_if.h"

struct kobjop_desc xenbus_otherend_changed_desc = {
	0, { &xenbus_otherend_changed_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc xenbus_localend_changed_desc = {
	0, { &xenbus_localend_changed_desc, (kobjop_t)xenbus_localend_changed }
};


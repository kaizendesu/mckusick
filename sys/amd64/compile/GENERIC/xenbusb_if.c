/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../xen/xenbus/xenbusb_if.m
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
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/taskqueue.h>
#include <xen/xenstore/xenstorevar.h>
#include <xen/xenbus/xenbusb.h>
#include "xenbusb_if.h"

struct kobjop_desc xenbusb_enumerate_type_desc = {
	0, { &xenbusb_enumerate_type_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc xenbusb_get_otherend_node_desc = {
	0, { &xenbusb_get_otherend_node_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc xenbusb_otherend_changed_desc = {
	0, { &xenbusb_otherend_changed_desc, (kobjop_t)xenbusb_otherend_changed }
};

struct kobjop_desc xenbusb_localend_changed_desc = {
	0, { &xenbusb_localend_changed_desc, (kobjop_t)xenbusb_localend_changed }
};


/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../dev/ata/ata_if.m
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
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/sema.h>
#include <sys/taskqueue.h>
#include <vm/uma.h>
#include <machine/bus.h>
#include <sys/ata.h>
#include <dev/ata/ata-all.h>
#include "ata_if.h"


static int ata_null_setmode(device_t dev, int target, int mode)
{

	if (mode > ATA_PIO_MAX)
		return (ATA_PIO_MAX);
	return (mode);
}

struct kobjop_desc ata_setmode_desc = {
	0, { &ata_setmode_desc, (kobjop_t)ata_null_setmode }
};


static int ata_null_getrev(device_t dev, int target)
{
	return (0);
}

struct kobjop_desc ata_getrev_desc = {
	0, { &ata_getrev_desc, (kobjop_t)ata_null_getrev }
};

struct kobjop_desc ata_reset_desc = {
	0, { &ata_reset_desc, (kobjop_t)ata_generic_reset }
};

struct kobjop_desc ata_reinit_desc = {
	0, { &ata_reinit_desc, (kobjop_t)kobj_error_method }
};


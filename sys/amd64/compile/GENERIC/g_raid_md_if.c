/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../geom/raid/g_raid_md_if.m
 * with
 *   makeobjops.awk
 *
 * See the source file for legal information
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/param.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/sbuf.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/systm.h>
#include <geom/geom.h>
#include <geom/raid/g_raid.h>
#include "g_raid_md_if.h"


static int
g_raid_md_create_default(struct g_raid_md_object *md,
    struct g_class *mp, struct g_geom **gp)
{

	return (G_RAID_MD_TASTE_FAIL);
}

static int
g_raid_md_create_req_default(struct g_raid_md_object *md,
    struct g_class *mp, struct gctl_req *req, struct g_geom **gp)
{

	return (G_RAID_MD_CREATE(md, mp, gp));
}

static int
g_raid_md_ctl_default(struct g_raid_md_object *md,
    struct gctl_req *req)
{

	return (-1);
}

static int
g_raid_md_volume_event_default(struct g_raid_md_object *md,
    struct g_raid_volume *vol, u_int event)
{

	return (-1);
}

static int
g_raid_md_free_disk_default(struct g_raid_md_object *md,
    struct g_raid_volume *vol)
{

	return (0);
}

static int
g_raid_md_free_volume_default(struct g_raid_md_object *md,
    struct g_raid_volume *vol)
{

	return (0);
}

struct kobjop_desc g_raid_md_create_desc = {
	0, { &g_raid_md_create_desc, (kobjop_t)g_raid_md_create_default }
};

struct kobjop_desc g_raid_md_create_req_desc = {
	0, { &g_raid_md_create_req_desc, (kobjop_t)g_raid_md_create_req_default }
};

struct kobjop_desc g_raid_md_taste_desc = {
	0, { &g_raid_md_taste_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc g_raid_md_ctl_desc = {
	0, { &g_raid_md_ctl_desc, (kobjop_t)g_raid_md_ctl_default }
};

struct kobjop_desc g_raid_md_event_desc = {
	0, { &g_raid_md_event_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc g_raid_md_volume_event_desc = {
	0, { &g_raid_md_volume_event_desc, (kobjop_t)g_raid_md_volume_event_default }
};

struct kobjop_desc g_raid_md_write_desc = {
	0, { &g_raid_md_write_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc g_raid_md_fail_disk_desc = {
	0, { &g_raid_md_fail_disk_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc g_raid_md_free_disk_desc = {
	0, { &g_raid_md_free_disk_desc, (kobjop_t)g_raid_md_free_disk_default }
};

struct kobjop_desc g_raid_md_free_volume_desc = {
	0, { &g_raid_md_free_volume_desc, (kobjop_t)g_raid_md_free_volume_default }
};

struct kobjop_desc g_raid_md_free_desc = {
	0, { &g_raid_md_free_desc, (kobjop_t)kobj_error_method }
};


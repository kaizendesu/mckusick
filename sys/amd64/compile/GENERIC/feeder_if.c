/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../dev/sound/pcm/feeder_if.m
 * with
 *   makeobjops.awk
 *
 * See the source file for legal information
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <dev/sound/pcm/sound.h>
#include "feeder_if.h"



	static int
	feeder_noinit(struct pcm_feeder* feeder)
	{
		return 0;
	}

	static int
	feeder_nofree(struct pcm_feeder* feeder)
	{
		return 0;
	}

	static int
	feeder_noset(struct pcm_feeder* feeder, int what, int value)
	{
		return -1;
	}

	static int
	feeder_noget(struct pcm_feeder* feeder, int what)
	{
		return -1;
	}


struct kobjop_desc feeder_init_desc = {
	0, { &feeder_init_desc, (kobjop_t)feeder_noinit }
};

struct kobjop_desc feeder_free_desc = {
	0, { &feeder_free_desc, (kobjop_t)feeder_nofree }
};

struct kobjop_desc feeder_set_desc = {
	0, { &feeder_set_desc, (kobjop_t)feeder_noset }
};

struct kobjop_desc feeder_get_desc = {
	0, { &feeder_get_desc, (kobjop_t)feeder_noget }
};

struct kobjop_desc feeder_feed_desc = {
	0, { &feeder_feed_desc, (kobjop_t)kobj_error_method }
};


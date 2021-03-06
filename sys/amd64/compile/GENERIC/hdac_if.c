/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../dev/sound/pci/hda/hdac_if.m
 * with
 *   makeobjops.awk
 *
 * See the source file for legal information
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/rman.h>
#include "hdac_if.h"

struct kobjop_desc hdac_get_mtx_desc = {
	0, { &hdac_get_mtx_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_codec_command_desc = {
	0, { &hdac_codec_command_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_alloc_desc = {
	0, { &hdac_stream_alloc_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_free_desc = {
	0, { &hdac_stream_free_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_start_desc = {
	0, { &hdac_stream_start_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_stop_desc = {
	0, { &hdac_stream_stop_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_reset_desc = {
	0, { &hdac_stream_reset_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_getptr_desc = {
	0, { &hdac_stream_getptr_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_stream_intr_desc = {
	0, { &hdac_stream_intr_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_unsol_alloc_desc = {
	0, { &hdac_unsol_alloc_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_unsol_free_desc = {
	0, { &hdac_unsol_free_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_unsol_intr_desc = {
	0, { &hdac_unsol_intr_desc, (kobjop_t)kobj_error_method }
};

struct kobjop_desc hdac_pindump_desc = {
	0, { &hdac_pindump_desc, (kobjop_t)kobj_error_method }
};


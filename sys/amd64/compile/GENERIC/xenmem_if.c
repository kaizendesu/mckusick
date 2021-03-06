/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from source file
 *   ../../../xen/xenmem/xenmem_if.m
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
#include "xenmem_if.h"


static struct resource *
xenmem_generic_alloc(device_t dev, device_t child, int *res_id,
    size_t size)
{
        device_t parent;

        parent = device_get_parent(dev);
        if (parent == NULL)
                return (NULL);
        return (XENMEM_ALLOC(parent, child, res_id, size));
}

static int
xenmem_generic_free(device_t dev, device_t child, int res_id,
    struct resource *res)
{
        device_t parent;

        parent = device_get_parent(dev);
        if (parent == NULL)
                return (ENXIO);
        return (XENMEM_FREE(parent, child, res_id, res));
}

struct kobjop_desc xenmem_alloc_desc = {
	0, { &xenmem_alloc_desc, (kobjop_t)xenmem_generic_alloc }
};

struct kobjop_desc xenmem_free_desc = {
	0, { &xenmem_free_desc, (kobjop_t)xenmem_generic_free }
};


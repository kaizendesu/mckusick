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


#ifndef _xenmem_if_h_
#define _xenmem_if_h_

/** @brief Unique descriptor for the XENMEM_ALLOC() method */
extern struct kobjop_desc xenmem_alloc_desc;
/** @brief A function implementing the XENMEM_ALLOC() method */
typedef struct resource * xenmem_alloc_t(device_t _dev, device_t _child,
                                         int *_res_id, size_t _size);
/**
 * @brief Request for unused physical memory regions.
 *
 * @param _dev          the device whose child was being probed.
 * @param _child        the child device which failed to probe.
 * @param _res_id       a pointer to the resource identifier.
 * @param _size         size of the required memory region.
 *
 * @returns             the resource which was allocated or @c NULL if no
 *                      resource could be allocated.
 */

static __inline struct resource * XENMEM_ALLOC(device_t _dev, device_t _child,
                                               int *_res_id, size_t _size)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenmem_alloc);
	return ((xenmem_alloc_t *) _m)(_dev, _child, _res_id, _size);
}

/** @brief Unique descriptor for the XENMEM_FREE() method */
extern struct kobjop_desc xenmem_free_desc;
/** @brief A function implementing the XENMEM_FREE() method */
typedef int xenmem_free_t(device_t _dev, device_t _child, int _res_id,
                          struct resource *_res);
/**
 * @brief Free physical memory regions.
 *
 * @param _dev          the device whose child was being probed.
 * @param _child        the child device which failed to probe.
 * @param _res_id       the resource identifier.
 * @param _res          the resource.
 *
 * @returns             0 on success, otherwise an error code.
 */

static __inline int XENMEM_FREE(device_t _dev, device_t _child, int _res_id,
                                struct resource *_res)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenmem_free);
	return ((xenmem_free_t *) _m)(_dev, _child, _res_id, _res);
}

#endif /* _xenmem_if_h_ */

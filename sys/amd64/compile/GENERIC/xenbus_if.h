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


#ifndef _xenbus_if_h_
#define _xenbus_if_h_

/** @brief Unique descriptor for the XENBUS_OTHEREND_CHANGED() method */
extern struct kobjop_desc xenbus_otherend_changed_desc;
/** @brief A function implementing the XENBUS_OTHEREND_CHANGED() method */
typedef void xenbus_otherend_changed_t(device_t _dev,
                                       enum xenbus_state _newstate);
/**
 * \brief Callback triggered when the state of the otherend
 *        of a split device changes.
 *
 * \param _dev       NewBus device_t for this XenBus device whose otherend's
 *                   state has changed..
 * \param _newstate  The new state of the otherend device.
 */

static __inline void XENBUS_OTHEREND_CHANGED(device_t _dev,
                                             enum xenbus_state _newstate)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenbus_otherend_changed);
	((xenbus_otherend_changed_t *) _m)(_dev, _newstate);
}

/** @brief Unique descriptor for the XENBUS_LOCALEND_CHANGED() method */
extern struct kobjop_desc xenbus_localend_changed_desc;
/** @brief A function implementing the XENBUS_LOCALEND_CHANGED() method */
typedef void xenbus_localend_changed_t(device_t _dev, const char * _path);
/**
 * \brief Callback triggered when the XenStore tree of the local end
 *        of a split device changes.
 *
 * \param _dev   NewBus device_t for this XenBus device whose otherend's
 *               state has changed..
 * \param _path  The tree relative sub-path to the modified node.  The empty
 *               string indicates the root of the tree was destroyed.
 */

static __inline void XENBUS_LOCALEND_CHANGED(device_t _dev, const char * _path)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenbus_localend_changed);
	((xenbus_localend_changed_t *) _m)(_dev, _path);
}

#endif /* _xenbus_if_h_ */

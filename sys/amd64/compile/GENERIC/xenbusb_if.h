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


#ifndef _xenbusb_if_h_
#define _xenbusb_if_h_

/** @brief Unique descriptor for the XENBUSB_ENUMERATE_TYPE() method */
extern struct kobjop_desc xenbusb_enumerate_type_desc;
/** @brief A function implementing the XENBUSB_ENUMERATE_TYPE() method */
typedef int xenbusb_enumerate_type_t(device_t _dev, const char *_type);
/**
 * \brief Enumerate all devices of the given type on this bus.
 *
 * \param _dev  NewBus device_t for this XenBus (front/back) bus instance.
 * \param _type String indicating the device sub-tree (e.g. "vfb", "vif")
 *              to enumerate. 
 *
 * \return  On success, 0. Otherwise an errno value indicating the
 *          type of failure.
 *
 * Devices that are found should be entered into the NewBus hierarchy via
 * xenbusb_add_device().  xenbusb_add_device() ignores duplicate detects
 * and ignores duplicate devices, so it can be called unconditionally
 * for any device found in the XenStore.
 */

static __inline int XENBUSB_ENUMERATE_TYPE(device_t _dev, const char *_type)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenbusb_enumerate_type);
	return ((xenbusb_enumerate_type_t *) _m)(_dev, _type);
}

/** @brief Unique descriptor for the XENBUSB_GET_OTHEREND_NODE() method */
extern struct kobjop_desc xenbusb_get_otherend_node_desc;
/** @brief A function implementing the XENBUSB_GET_OTHEREND_NODE() method */
typedef int xenbusb_get_otherend_node_t(device_t _dev,
                                        struct xenbus_device_ivars *_ivars);
/**
 * \brief Determine and store the XenStore path for the other end of
 *        a split device whose local end is represented by ivars.
 *
 * If successful, the xd_otherend_path field of the child's instance
 * variables must be updated.
 *
 * \param _dev    NewBus device_t for this XenBus (front/back) bus instance.
 * \param _ivars  Instance variables from the XenBus child device for
 *                which to perform this function.
 *
 * \return  On success, 0. Otherwise an errno value indicating the
 *          type of failure.
 */

static __inline int XENBUSB_GET_OTHEREND_NODE(device_t _dev,
                                              struct xenbus_device_ivars *_ivars)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_dev)->ops,xenbusb_get_otherend_node);
	return ((xenbusb_get_otherend_node_t *) _m)(_dev, _ivars);
}

/** @brief Unique descriptor for the XENBUSB_OTHEREND_CHANGED() method */
extern struct kobjop_desc xenbusb_otherend_changed_desc;
/** @brief A function implementing the XENBUSB_OTHEREND_CHANGED() method */
typedef void xenbusb_otherend_changed_t(device_t _bus, device_t _child,
                                        enum xenbus_state _state);
/**
 * \brief Handle a XenStore change detected in the peer tree of a child
 *        device of the bus.
 *
 * \param _bus       NewBus device_t for this XenBus (front/back) bus instance.
 * \param _child     NewBus device_t for the child device whose peer XenStore
 *                   tree has changed.
 * \param _state     The current state of the peer.
 */

static __inline void XENBUSB_OTHEREND_CHANGED(device_t _bus, device_t _child,
                                              enum xenbus_state _state)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_bus)->ops,xenbusb_otherend_changed);
	((xenbusb_otherend_changed_t *) _m)(_bus, _child, _state);
}

/** @brief Unique descriptor for the XENBUSB_LOCALEND_CHANGED() method */
extern struct kobjop_desc xenbusb_localend_changed_desc;
/** @brief A function implementing the XENBUSB_LOCALEND_CHANGED() method */
typedef void xenbusb_localend_changed_t(device_t _bus, device_t _child,
                                        const char * _path);
/**
 * \brief Handle a XenStore change detected in the local tree of a child
 *        device of the bus.
 *
 * \param _bus    NewBus device_t for this XenBus (front/back) bus instance.
 * \param _child  NewBus device_t for the child device whose peer XenStore
 *                tree has changed.
 * \param _path   The tree relative sub-path to the modified node.  The empty
 *                string indicates the root of the tree was destroyed.
 */

static __inline void XENBUSB_LOCALEND_CHANGED(device_t _bus, device_t _child,
                                              const char * _path)
{
	kobjop_t _m;
	KOBJOPLOOKUP(((kobj_t)_bus)->ops,xenbusb_localend_changed);
	((xenbusb_localend_changed_t *) _m)(_bus, _child, _path);
}

#endif /* _xenbusb_if_h_ */

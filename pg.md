# Walkthrough of FreeBSD 11's Paging System

## Contents

1. Code Flow
2. Reading Checklist
3. Important Data Structures
4. Code Walkthrough

## Code Flow

This section describes the code flow in four level deep tree structure.
Each level of the tree corresponds to a function call with its own
unique stack frame, where the only exception is for one-liner functions
that call a function within a return.

```txt
trap
	trap_pfault
		vm_fault
			vm_fault_hold
				vm_map_lookup
					vm_map_lock_read
					vm_map_lookup_entry
					vm_object_shadow
				vm_object_reference_locked
				vm_object_pip_add
				vm_page_lookup
				vm_page_lock
				vm_page_remque
					vm_page_dequeue
				vm_page_unlock
				vm_page_xbusy
				vm_page_alloc
					vm_radix_lookup_le
					vm_page_cache_lookup
					vm_reserv_alloc_page
					vm_phys_alloc_pages
					vm_reserv_reclaim_inactive



					pagedaemon_wakup
				vm_map_entry_behavior
				vm_pager_get_pages
				pmap_zero_page
					pagezero
				pmap_enter
				vm_page_activate
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: trap.c
	trap							----
	trap_pfault						----

File: vm_fault.c
	vm_fault						----
	vm_fault_hold					----

File: vm_map.c
	vm_map_lookup					----
	vm_map_lock_read				----
	vm_map_lookup_entry				++--
	vm_map_entry_behavior			----

File: vm_object.c
	vm_object_shadow				----
	vm_object_reference_locked		----
	vm_object_pip_add				----

File: vm_page.c
	vm_page_lookup					----
	vm_page_lock					----
	vm_page_remque					----
	vm_page_dequeue					----
	vm_page_unlock					----
	vm_page_xbusy					----
	vm_page_alloc					----
	vm_page_cache_lookup			----
	vm_page_activate				----

File: vm_radix.c
	vm_radix_lookup_le				----

File: vm_reserv.c
	vm_reserv_alloc_page			----
	vm_reserv_reclaim_inactive		----

File: vm_phys.c
	vm_phys_alloc_pages				----

File: vm_pageout.c
	pagedaemon_wakeup				----

File: vm_pager.c
	vm_pager_get_pages				----

File: pmap.c
	pmap_zero_page					----
	pmap_copy_page					----
	pmap_enter						----

File: support.S
	pagezero						----
```

## Important Data Structures and Algorithms

### Fault State Structure

```c
/* From /sys/vm/vm_fault.c */

struct faultstate {
	vm_page_t m;
	vm_object_t object;
	vm_pindex_t pindex;
	vm_page_t first_m;
	vm_object_t first_object;
	vm_pindex_t first_pindex;
	vm_map_t map;
	vm_map_entry_t entry;
	int lookup_still_valid;
	struct vnode *vp;
};
```

## Code Walkthrough

### Pseudo Code Overview 

### Documented Code

```c
```

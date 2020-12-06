# Walkthrough of FreeBSD 11's Memory Map System Call

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
sys_mmap
	vn_mmap
		vm_mmap_vnode
			vm_pager_allocate
				vnode_pager_alloc
					vm_object_allocate
						_vm_object_allocate
		vm_mmap_object
			vm_map_find
				vm_map_findspace
					vm_map_entry_splay
			vm_map_insert
				vm_map_lookup_entry
				vm_map_entry_create
				vm_map_entry_link
					vm_map_entry_splay
					vm_map_entry_set_max_free
				vm_map_simplify_entry
				vm_map_pmap_enter
					vm_page_find_least
					pmap_enter_object
						pmap_enter_quick_locked
						pmap_try_insert_pv_entry
						pte_store
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: vm_mmap.c
	sys_mmap					----
	vm_mmap_vnode				----
	vm_mmap_object				----
	vm_map_find					----

File: vfs_vnops.c
	vn_mmap						----

File: vm_pager.c
	vm_pager_allocate			----
	vnode_pager_alloc			----

File: vm_object.c
	vm_object_allocate			----
	_vm_object_allocate			----

File: vm_map.c
	vm_map_find					----
	vm_map_findspace			----
	vm_map_entry_splay			----
	vm_map_insert				----
	vm_map_lookup_entry			----
	vm_map_entry_create			----
	vm_map_entry_link			----
	vm_map_entry_set_max_free	----
	vm_map_simplify_entry		----
	vm_map_pmap_enter			----

File: vm_page.c
	vm_page_find_least			----

File: pmap.c
	pmap_enter_object			----
	pmap_enter_quick_locked		----
	pmap_try_insert_pv_entry	----

File: pmap.h
	pte_store					----
```

## Important Data Structures

## Code Walkthrough

### Pseudo Code Overview 

### Documented Code

```c
```

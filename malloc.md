# Walkthrough of FreeBSD 11's Memory Allocation Functions

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
malloc

free
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: kern_malloc.c
    malloc            ----
    free              ----
```

## Important Data Structures

### Malloc Type Structures

```c
/* From /sys/sys/malloc.h */

/*
 * Two malloc type structures are present: malloc_type, which is used by a
 * type owner to declare the type, and malloc_type_internal, which holds
 * malloc-owned statistics and other ABI-sensitive fields, such as the set of
 * malloc statistics indexed by the compile-time MAXCPU constant.
 * Applications should avoid introducing dependence on the allocator private
 * data layout and size.
 *
 * The malloc_type ks_next field is protected by malloc_mtx.  Other fields in
 * malloc_type are static after initialization so unsynchronized.
 *
 * Statistics in malloc_type_stats are written only when holding a critical
 * section and running on the CPU associated with the index into the stat
 * array, but read lock-free resulting in possible (minor) races, which the
 * monitoring app should take into account.
 */
struct malloc_type_stats {
	uint64_t	mts_memalloced;	/* Bytes allocated on CPU. */
	uint64_t	mts_memfreed;	/* Bytes freed on CPU. */
	uint64_t	mts_numallocs;	/* Number of allocates on CPU. */
	uint64_t	mts_numfrees;	/* number of frees on CPU. */
	uint64_t	mts_size;	/* Bitmask of sizes allocated on CPU. */
	uint64_t	_mts_reserved1;	/* Reserved field. */
	uint64_t	_mts_reserved2;	/* Reserved field. */
	uint64_t	_mts_reserved3;	/* Reserved field. */
};

/*
 * Index definitions for the mti_probes[] array.
 */
#define DTMALLOC_PROBE_MALLOC		0
#define DTMALLOC_PROBE_FREE		1
#define DTMALLOC_PROBE_MAX		2

struct malloc_type_internal {
	uint32_t	mti_probes[DTMALLOC_PROBE_MAX];
					/* DTrace probe ID array. */
	u_char		mti_zone;
	struct malloc_type_stats	mti_stats[MAXCPU];
};

/*
 * Public data structure describing a malloc type.  Private data is hung off
 * of ks_handle to avoid encoding internal malloc(9) data structures in
 * modules, which will statically allocate struct malloc_type.
 */
struct malloc_type {
	struct malloc_type *ks_next;	/* Next in global chain. */
	u_long		 ks_magic;	/* Detect programmer error. */
	const char	*ks_shortdesc;	/* Printable type name. */
	void		*ks_handle;	/* Priv. data, was lo_class. */
};
```

## Code Walkthrough

### Malloc

```c
```

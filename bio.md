# Walkthrough of FreeBSD 11's Buffer I/O System

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
breadn_flags
	getblk
		gbincore
		bremfree
		getnewbuf
			buf_scan
				buf_recycle
		bgetvp	
	bufstrategy
		ufs_strategy
			ffs_geom_strategy
				g_vfs_strategy
					g_io_request
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: vfs_bio.c
	breadn_flags			----
	getblk					----
	getnewbuf				----
	buf_scan				----
	buf_recycle				----
	allocbuf				----
	bufstrategy				----

File: vfs_subr.c
	gbincore				----
	bremfree				----
	getnewbuf				----
	bgetvp					----

File: ufs_vnops.c
	ufs_strategy			----

File: ffs_vfsops.c
	ffs_geom_strategy		----

File: geom_vfs.c
	g_vfs_strategy			----

File: geom_io.c
	g_io_request			----
```

## Important Data Structures

## Code Walkthrough

### Pseudo Code Overview

### Documented Code

```c
```

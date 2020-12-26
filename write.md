# Walkthrough of FreeBSD 11's Write System Call (Excluding Assembly Stubs)

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
sys_write
    kern_writev
        fget_write
        _fget
            fget_unlocked
        dofilewrite
            vn_io_fault (fo_write)
                foffset_lock_uio
                vn_write
                    get_advice
                    vn_lock
                    _vn_lock
                    ffs_write (VOP_WRITE)
                        ffs_balloc_ufs2 (UFS_BALLOC)
                            ufs_getlbns
                        ffs_alloc
                            ffs_hashalloc
                                ffs_alloccg
                                    ffs_alloccgblk
                                    ffs_mapsearch
                    vop_stdunlock (VOP_UNLOCK)
                    vop_stdadvise (VOP_ADVISE)
                foffset_unlock_uio
        fdrop
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: sys_generic.c
    sys_write           ++--
    kern_writev         ++--
    dofilewrite         ++--

File: kern_descrip.c
    fget_write          ++--
    _fget               ++--
    fget_unlocked       ++--

File: vfs_vnops.c 
    vn_io_fault         +---
    foffset_lock_uio    ++--
    do_vn_io_fault      ++--
    vn_write            ++--
    vn_start_write      +---
    get_advice          ++--
    _vn_lock            ++--
    vn_io_fault_uiomove ----
    vn_io_fault_pgmove  ----
   foffset_unlock_uio   ++--
	
File: ffs_vnops.c
    ffs_write           ++--

File: ffs_balloc.c
    ffs_balloc_ufs2     ----

File: ufs_bmap.c
    ufs_getlbns         ----

File: ffs_alloc.c
    ffs_alloc           ----
    ffs_hashalloc       ----
    ffs_alloccg         ----
    ffs_alloccgblk      ----
    ffs_mapsearch       ----

File: ffs_inode.c
    ffs_update          ----

File: vfs_default.c
    vop_stdunlock       ----
    vop_stdadvise       ----
```

## Important Data Structures

### I/O Data Structures

```c
/* From /sys/sys/_iovec.h */

struct iovec {
	void	*iov_base;	/* Base address. */
	size_t	 iov_len;	/* Length. */
};

/* From /sys/sys/uio.h */

struct uio {
	struct	iovec *uio_iov;		/* scatter/gather list */
	int	uio_iovcnt;		/* length of scatter/gather list */
	off_t	uio_offset;		/* offset in target object */
	ssize_t	uio_resid;		/* remaining bytes to process */
	enum	uio_seg uio_segflg;	/* address space */
	enum	uio_rw uio_rw;		/* operation */
	struct	thread *uio_td;		/* owner */
};
```

### File Data Structures

```c
/* From /sys/sys/filedesc.h */

struct filecaps {
	cap_rights_t	 fc_rights;	/* per-descriptor capability rights */
	u_long		*fc_ioctls;	/* per-descriptor allowed ioctls */
	int16_t		 fc_nioctls;	/* fc_ioctls array size */
	uint32_t	 fc_fcntls;	/* per-descriptor allowed fcntls */
};

struct filedescent {
	struct file	*fde_file;	/* file structure for open file */
	struct filecaps	 fde_caps;	/* per-descriptor rights */
	uint8_t		 fde_flags;	/* per-process open file flags */
	seq_t		 fde_seq;	/* keep file and caps in sync */
};
#define	fde_rights	fde_caps.fc_rights
#define	fde_fcntls	fde_caps.fc_fcntls
#define	fde_ioctls	fde_caps.fc_ioctls
#define	fde_nioctls	fde_caps.fc_nioctls
#define	fde_change_size	(offsetof(struct filedescent, fde_seq))

struct fdescenttbl {
	int	fdt_nfiles;		/* number of open files allocated */
	struct	filedescent fdt_ofiles[0];	/* open files */
};
#define	fd_seq(fdt, fd)	(&(fdt)->fdt_ofiles[(fd)].fde_seq)

/*
 * This structure is used for the management of descriptors.  It may be
 * shared by multiple processes.
 */
#define NDSLOTTYPE	u_long

struct filedesc {
	struct	fdescenttbl *fd_files;	/* open files table */
	struct	vnode *fd_cdir;		/* current directory */
	struct	vnode *fd_rdir;		/* root directory */
	struct	vnode *fd_jdir;		/* jail root directory */
	NDSLOTTYPE *fd_map;		/* bitmap of free fds */
	int	fd_lastfile;		/* high-water mark of fd_ofiles */
	int	fd_freefile;		/* approx. next free file */
	u_short	fd_cmask;		/* mask for file creation */
	int	fd_refcnt;		/* thread reference count */
	int	fd_holdcnt;		/* hold count on structure + mutex */
	struct	sx fd_sx;		/* protects members of this struct */
	struct	kqlist fd_kqlist;	/* list of kqueues on this filedesc */
	int	fd_holdleaderscount;	/* block fdfree() for shared close() */
	int	fd_holdleaderswakeup;	/* fdfree() needs wakeup */
};

/* From /sys/sys/file.h */

struct file {
	void		*f_data;	/* file descriptor specific data */
	struct fileops	*f_ops;		/* File operations */
	struct ucred	*f_cred;	/* associated credentials. */
	struct vnode 	*f_vnode;	/* NULL or applicable vnode */
	short		f_type;		/* descriptor type */
	short		f_vnread_flags; /* (f) Sleep lock for f_offset */
	volatile u_int	f_flag;		/* see fcntl.h */
	volatile u_int 	f_count;	/* reference count */
	/*
	 *  DTYPE_VNODE specific fields.
	 */
	int		f_seqcount;	/* (a) Count of sequential accesses. */
	off_t		f_nextoff;	/* next expected read/write offset. */
	union {
		struct cdev_privdata *fvn_cdevpriv;
					/* (d) Private data for the cdev. */
		struct fadvise_info *fvn_advice;
	} f_vnun;
	/*
	 *  DFLAG_SEEKABLE specific fields
	 */
	off_t		f_offset;
	/*
	 * Mandatory Access control information.
	 */
	void		*f_label;	/* Place-holder for MAC label. */
};
```

## Code Walkthrough

### Pseudo Code Overview 

**sys_write**: Creates and initializes kernel i/o structs and calls kern\_writev.

* Checks size of buffer
* Fills uio struct with syscall args
* Creates iovec to point to uio struct
* Calls kern\_writev
* Returns kern\_writev's error value to syscall code

**kern_writev**: Obtains the file entry and calls dowritefile.

* Initializes cap rights structure
* Obtains file entry ptr from fget\_write
* Uses file ent ptr to call dofilewrite
* Calls fdrop to drop ref on file ent
* Returns the error value from dofilewritefile to sys\_write

**fget_write**: Wrapper function to \_fget that passes the FWRITE argument in particular.

* Calls \_fget with FWRITE as the flags arg and NULL for seqp
* Returns file entry to kern\_writev

\_**fget**: Calls fget\_unlocked to obtain the appropriate file entry and returns it to fget\_write.

* Obtains file entry with fget\_unlocked
* Asserts file status flags match FWRITE and returns EBADF if false
* Returns file entry ptr to kern\_writev

**fget\_unlocked**: Obtains the file entry locklessly by looping until it can obtain the file entry with a nonzero ref count.

* Checks the refcount, forcing a reload if it is zero
* Checks if we have reallocated the descriptor table after atomically incrementing the ref count in case of preemption.
* Returns the file entry to \_fget


**dofilewrite**: Fills the rest of the uio structure, calls bwillwrite to alert the filesystem, calls vn\_io\_fault and modifies its return value if necessary, sets the number of characters written into td\_retval, and finally returns vn\)io\_fault's retval to kern\_writev.

* Sets iovec's operation, thread, and offset
* Calls bwillwrite() for regular file vnodes
* Calls vn\_io\_fault
* If the syscall was interrupted before completing, sets the error value to zero
* Sets the number of characters written to td\_retval
* Returns the possibly modified error value of vn\_io\_fault to kern\_writev

**vn\_io\_fault**:

* Sets _doio_ to vn\_write
* Obtains the file offset by calling foffset\_lock\_uio
* Calls vn\_write
* Updates the file offset by calling foffset\_unlock\_uio
* Returns the retval from vn\_write to dofilewrite

**foffset\_lock\_uio**: Uses a sleepable mutex from the mutex pool to acquire the lock on the file's offset.

* Calls foffset\_lock to obtain the file offset

**do\_vn\_io\_fault**: Returns true if the i/o is in userspace, the vnode type is regular file, the userspace pointer to the mount point is not NULL, if page faults are disabled in read operations (MNTK\_NO\_IOPF), and if vn\_io\_fault\_enable is set.

**vn_write**: Alerts the filesystem about the write operation, checks whether the vnode is suspended for writing, obtains i/o advice from get\_advice, calls ffs\_write and returns its error value to dofilewrite. 

* Calls bwillwrite to alert filesystem
* Uses file status flags assign io flags
* Calls vn\_start\_write for everything except character devices
* Obtains i/o advice from get\_advice
* Calls ffs\_write
* Flushes pages/buffers with vop\_stdavice for POSIX\_FADV\_NOREUSE
* Returns ffs\_write's error value to dofilewrite;

**vn\_start\_write**:

**get_advice**: Checks whether i/o operation conforms to the i/o range specified in the file entry's f\_advice structure, and assigns that structures advice if it does not.

* Sets retval to default no advice (POSIX\_FADV\_NORMAL)
* Obtains sleepable mutex from the mutex pool and locks it
* Modifies retval if the i/o doesn't conform to the file entry's i/o range in fp-\>f\_advice
* Unlocks the mutex and returns reval to vn\_write

\_**vn\_lock**: Attempts to acquire a lock on a vnode, looping until it is successful and potentially returning doomed vnodes if LK\_RETRY is set.

**ffs_write**: Checks if the file size after write is too large for the file system, determines whether the write will require an update to the vnode and disk inode's size fields, loops until the i/o is complete or there is an error, and returns to vn\_write.

* Modifies uio\_offset for IO\_APPEND flags
* Checks if file size after write is larger than max file size for the vnode's filesystem
* Loop: Obtains logical block number and block offset
* Loop: Calculates xfersize as the minimum of characters to write and
		leftover unwritten characters in the buffer to determine whether
		to set the BA_CLRBUF flag for read-mod-writes. Also call 
		vm\_pager\_setsize if the write will cause the file to grow
* Loop: Allocates a buffer with ufs\_balloc\_ufs2
* Loop: Updates the disk inode if the write will cause the file to grow
* Loop: Fills in the buffer with either a uiomove or pgmove 
* Loop: Clears the buf if there was an error in the prev step
* Loop: Calls bwrite for synch writes, bawrite for asynch writes,
			or else in general clustered writes via cluster_write, 
			or delayed writes otherwise via bdwrite
* Loop: Repeat the loop if not finished and no errors
* Truncates the file if we were not able to complete the entire write with IO\_UNIT set
* Returns something to vn\_write

**ffs\_balloc\_ufs2**:

**vn\_io\_fault\_uiomove**:

**vn\_io\_fault\_pgmove**:

**ffs_update**:

**vop_stdunlock**:


**vop_stdadvise**: Deactives any pages used in the write for POSIX\_FADV\_DONTNEED, does nothing for POSIX\_FADV\_WILLNEED, and returns EINVAL otherwise.


**foffset\_unlock\_uio**: Uses a sleepable mutex from the mutex pool to update the file's offset and wake ups any threads waiting on the file offset lock.

* Calls foffset\_unlock to update the file offset and next offset
* Calls wakeup if there are any threads waiting on the file offset lock

**fdrop**:

### Documented Code

```c
```

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
	sys_write			++--
	kern_writev			++--
	dofilewrite			++--

File: kern_descrip.c
	fget_write			++--
	_fget				++--
	fget_unlocked		++--

File: vfs_vnops.c 
	vn_io_fault			+---
	foffset_lock_uio	++--
	do_vn_io_fault		++--
	vn_write			++--
	get_advice			++--
	_vn_lock			----
	foffset_unlock_uio	++--
	
File: ffs_vnops.c
	ffs_write			+---

File: ffs_balloc.c
	ffs_balloc_ufs2		----

File: ufs_bmap.c
	ufs_getlbns			----

File: ffs_alloc.c
	ffs_alloc			----
	ffs_hashalloc		----
	ffs_alloccg			----
	ffs_alloccgblk		----
	ffs_mapsearch		----

File: vfs_default.c
	vop_stdunlock		----
	vop_stdadvise		----
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

**fget_unlocked**: Obtains the file entry locklessly by looping until it can obtain the file entry with a nonzero ref count.
	* Checks the refcount, forcing a reload if it is zero
	* Checks if we have reallocated the descriptor table after
	  atomically incrementing the ref count in case of preemption.
	* Returns the file entry to \+fget
}

**dofilewrite**: Fills the rest of the uio structure, calls bwillwrite to alert the filesystem, calls vn\_io\_fault and modifies its return value if necessary, sets the number of characters written into td\_retval, and finally returns vn\)io\_fault's retval to kern\_writev.
	* Sets iovec's operation, thread, and offset
	* Calls bwillwrite() for regular file vnodes
	* Calls vn\_io\_fault
	* If the syscall was interrupted before completing, sets the error
	  value to zero
	* Sets the number of characters written to td\_retval
	* Returns the possibly modified error value of vn\_io\_fault to
	  kern\_writev

**vn_io_fault**:
	* Sets _doio_ to vn\_write
	* Obtains the file offset by calling foffset\_lock\_uio
	* calls vn\_write
	* Updates the file offset by calling foffset\_unlock\_uio
	* Returns the retval from vn\_write to dofilewrite

**foffset_lock_uio**: Uses a sleepable mutex from the mutex pool to acquire the lock on the file's offset.
	* Calls foffset\_lock to obtain the file offset

**do_vn_io_fault**: Returns true if the i/o is in userspace, the vnode type is regular file, the userspace pointer to the mount point is not NULL, if page faults are disabled in read operations (MNTK\_NO\_IOPF), and if vn\_io\_fault\_enable is set.

static int
vn_write(fp, uio, active_cred, flags, td)
{
	call bwillwrite() to alert filesystem;
	use file status flags assign io flags;
	call vn_start_write() for reg files and block devs;
	obtain i/o advice from get_advice if i/o doesn't conform
		to file ent's i/o range;
	call ffs_write to complete write at filesystem level;
	flush pages and buffers with vop_stdavice;
	return nb of chars written to dofilewrite;
}

static int
get_advice(struct file *fp, struct uio *uio)
{
	set retval to default no advice (POSIX_FADV_NORMAL);
	obtain sleepable mtx from pool;
	lock mtx;
	set retval to fp->f_advice->fa_advice if the params
		in uio don't not fit within f_advice's i/o range;
	unlock mtx;
	return retval to vn_write;
}

vn_lock

_vn_lock

static int
ffs_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	switch through vnode flags for error checking or mod uio_offset;
	ensure file after write will not be too big;
	
	for (no error or words to write)
	{
		obtain lbn, blk offset, and xfersize;
		extend file size if necessary with vnode_pager_setsize();
		set BA_CLRBUF if blk will have empty portions after write;
		call ufs_balloc_ufs2() to alloc block;
		xfer ioflags to buffer;
		update file size in dinode if necessary;
		write contents to buffer or pgs depending on if buf is mapped
			into kern;
		clear buf if there was an error in prev step;
		call bwrite for synch writes, use bawrite for asynch writes,
			clustered writes in general via cluster_write, or delayed
			writes otherwise via bdwrite;
	}
	truncate the file if we were not able to complete the entire write
		with IO_UNIT set;
	return nb of chars written to vn_write;
}

/*
 * Balloc defines the structure of file system storage
 * by allocating the physical blocks on a device given
 * the inode and the logical block number in a file.
 * This is the allocation strategy for UFS2. Above is
 * the allocation strategy for UFS1.
 */
int
ffs_balloc_ufs2(struct vnode *vp, off_t startoffset, int size,
				struct ucred *cred, int flags, struct buf **bpp)
{
	
}

vop_stdunlock (VOP_UNLOCK)

int
vop_stdadvise(struct vop_advise_args *ap)
{
	deactivate pages for POSIX_FADV_DONTNEED or keep them for
		POSIX_FADV_WILLNEED. EINVAL otherwise;
}

**foffset_unlock_uio**: Uses a sleepable mutex from the mutex pool to update the file's offset and wake ups any threads waiting on the file offset lock.
	* Calls foffset_unlock to update the file offset and next offset
	* Calls wakeup if there are any threads waiting on the file offset lock

**fdrop**:

### Documented Code

```c
```

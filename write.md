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
	sys_write			+++-
	kern_writev			+++-
	dofilewrite			+---

File: kern_descrip.c
	fget_write			+++-
	_fget				+---
	fget_unlocked		----

File: vfs_vnops.c 
	vn_io_fault			+---
	foffset_lock_uio	+---
	vn_write			----
	get_advice			----
	_vn_lock			----
	foffset_unlock_uio	+---
	
File: ffs_vnops.c
	ffs_write			----

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

```c
int
sys_write(td, uap)
{
	Check size of buffer;
	fill uio struct with syscall args;
	create iovec to point to uio struct;
	call kern_writev;
	return to syscall code;
}

int
kern_writev(struct thread *td, int fd, struct uio *auio)
{
	init cap rights structure;
	Obtain file ent ptr from fget_write;
	Use file ent ptr to call dofilewrite;
	call fdrop to drop ref on file ent;
	return to sys_write; 
}

int
fget_write(struct thread *td, int fd, cap_rights_t *rightsp, struct file *fpp)
{
	call _fget with flags == FWRITE and and seqp == NULL;
	return result to kern_writev;
}

static __inline int
_fget(struct thread *td, int fd, struct file **fpp, int flags,
		cap_rights_t *needrightsp, seq_t *seqp)
{
	use td to obtain filedesc ptr (first entry?);
	obtain file ent with fget_unlocked using filedesc ptr;
	assert file status flags match flags arg. return EBADF if false;
	return file ent ptr to kern_writev;
}

int
fget_unlocked(struct filedesc *fdp, int fd, cap_rights_t *needrightsp,
				struct file **fpp, seq_t *seqp)
{
	use filedesc to obtain filedesctbl ptr;
	for(;;) {
		obtain filedescent using fd arg;
		if refcount is 0, force reload of filedescent;
		check if we realloc filedesc tbl after incr ref,
			if we did, drop ref and try again;
		if filedesc tbl was not mod, break;
	}
	return file ent to _fget;
}

static int
dofilewrite(td, fd, fp, auio, offset, flags)
{
	set iovec's operation to UIO_WRITE;
	set iovec's td and offset;
	call bwillwrite() if the vnode type is file and the foffset 
		mutex is unlocked;
	call vn_io_fault to complete the write operation;
	if we were int before completing the write op, set error to
		0 anyways;
	return nb of chars written to kern_writev;
}

/*
 * The vn_io_fault() is a wrapper around vn_read() and vn_write() to
 * prevent the following deadlock:
 *
 * Assume that the thread A reads from the vnode vp1 into userspace
 * buffer buf1 backed by the pages of vnode vp2.  If a page in buf1 is
 * currently not resident, then system ends up with the call chain
 *   vn_read() -> VOP_READ(vp1) -> uiomove() -> [Page Fault] ->
 *     vm_fault(buf1) -> vnode_pager_getpages(vp2) -> VOP_GETPAGES(vp2)
 * which establishes lock order vp1->vn_lock, then vp2->vn_lock.
 * If, at the same time, thread B reads from vnode vp2 into buffer buf2
 * backed by the pages of vnode vp1, and some page in buf2 is not
 * resident, we get a reversed order vp2->vn_lock, then vp1->vn_lock.
 *
 * To prevent the lock order reversal and deadlock, vn_io_fault() does
 * not allow page faults to happen during VOP_READ() or VOP_WRITE().
 * Instead, it first tries to do the whole range i/o with pagefaults
 * disabled. If all pages in the i/o buffer are resident and mapped,
 * VOP will succeed (ignoring the genuine filesystem errors).
 * Otherwise, we get back EFAULT, and vn_io_fault() falls back to do
 * i/o in chunks, with all pages in the chunk prefaulted and held
 * using vm_fault_quick_hold_pages().
 *
 * Filesystems using this deadlock avoidance scheme should use the
 * array of the held pages from uio, saved in the curthread->td_ma,
 * instead of doing uiomove().  A helper function
 * vn_io_fault_uiomove() converts uiomove request into
 * uiomove_fromphys() over td_ma array.
 *
 * Since vnode locks do not cover the whole i/o anymore, rangelocks
 * make the current i/o request atomic with respect to other i/os and
 * truncations.
 */

static int
vn_io_fault(struct file *fp, struct uio *uio, struct ucred *active_cred,
			int flags, struct thread *td)
{
	set doio = &vn_write;
	obtain the file offset by calling foffset_lock_uio;
	call vn_write;
	update the file offset by calling foffset_unlock_uio;
	return the retval from vn_write to dofilewrite;
}

void
foffset_lock_uio(struct file *fp, struct *uio, int flags)
{
	/*
	 * foffset_lock uses a sleepable mtx from a mtx pool to
	 * acquire a lock on the file offset. This works simply
	 * by sleeping foffset lock is available.
	 *
	 * FOF_OFFSET means use the offset value in uio arg.
	 */
	Call foffset_lock to assign the offset since
		flags == 0x2 and FOF_OFFSET == 0x1;
}

vn_write

get_advice

vn_lock

_vn_lock

ffs_write (VOP_WRITE)

vop_stdunlock (VOP_UNLOCK)

vop_stdadvise (VOP_ADVISE)

void
foffset_unlock_uio(struct file *fp, struct uio *uio)
{
	/*
	 * foffset_unlock uses a sleepable mtx from a mtx pool to 
	 * set f_offset and f_nextoff, checks to see if we lost the
	 * lock on foffset, and calls wakeup if any td's are waiting
	 * on the foffset lock.
	 *
	 * FOF_OFFSET means use the offset value in uio arg.
	 */
	call foffset_unlock since flags == 0x2 and FOF_OFFSET == 1;
}

fdrop
```

### Documented Code

```c
```

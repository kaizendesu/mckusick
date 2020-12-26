# Walkthrough of FreeBSD 11's Open System Call (Excluding Assembly Stubs)

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
sys_open
kern_openat	
    falloc_noinstall
    vn_open
    vn_open_cred
        namei
        vn_open_vnode
    finstall
        fdalloc
        _finstall
    fdrop
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: vfs_syscall.c
    sys_open            +++
    kern_openat         +++

/* Was tired when doing this file, double check everything */
File: kern_descrip.c
    falloc_noinstall    ++-
    finstall            ++-
    fdalloc             ++-
    _finstall           ++-
    _fdrop              +--

/* vn_open_vnode will require closer look in future */
File: vfs_vnops.c
    vn_open             +++ 
    vn_open_cred        +++
    vn_open_vnode       +-+
```

## Important Data Structures

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

/*
 * Kernel descriptor table.
 * One entry for each open kernel vnode and socket.
 *
 * Below is the list of locks that protects members in struct file.
 *
 * (a) f_vnode lock required (shared allows both reads and writes)
 * (f) protected with mtx_lock(mtx_pool_find(fp))
 * (d) cdevpriv_mtx
 * none	not locked
 */

struct fadvise_info {
	int		fa_advice;	/* (f) FADV_* type. */
	off_t		fa_start;	/* (f) Region start. */
	off_t		fa_end;		/* (f) Region end. */
};


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

struct fileops {
	fo_rdwr_t	*fo_read;
	fo_rdwr_t	*fo_write;
	fo_truncate_t	*fo_truncate;
	fo_ioctl_t	*fo_ioctl;
	fo_poll_t	*fo_poll;
	fo_kqfilter_t	*fo_kqfilter;
	fo_stat_t	*fo_stat;
	fo_close_t	*fo_close;
	fo_chmod_t	*fo_chmod;
	fo_chown_t	*fo_chown;
	fo_sendfile_t	*fo_sendfile;
	fo_seek_t	*fo_seek;
	fo_fill_kinfo_t	*fo_fill_kinfo;
	fo_mmap_t	*fo_mmap;
	fo_flags_t	fo_flags;	/* DFLAG_* below */
};

#define DFLAG_PASSABLE	0x01	/* may be passed via unix sockets. */
#define DFLAG_SEEKABLE	0x02	/* seekable / nonsequential */
```

### Vnode Data Structure

```c
/* From /sys/sys/vnode.h */

struct vnode {
	/*
	 * Fields which define the identity of the vnode.  These fields are
	 * owned by the filesystem (XXX: and vgone() ?)
	 */
	const char *v_tag;			/* u type of underlying data */
	struct	vop_vector *v_op;		/* u vnode operations vector */
	void	*v_data;			/* u private data for fs */

	/*
	 * Filesystem instance stuff
	 */
	struct	mount *v_mount;			/* u ptr to vfs we are in */
	TAILQ_ENTRY(vnode) v_nmntvnodes;	/* m vnodes for mount point */

	/*
	 * Type specific fields, only one applies to any given vnode.
	 * See #defines below for renaming to v_* namespace.
	 */
	union {
		struct mount	*vu_mount;	/* v ptr to mountpoint (VDIR) */
		struct socket	*vu_socket;	/* v unix domain net (VSOCK) */
		struct cdev	*vu_cdev; 	/* v device (VCHR, VBLK) */
		struct fifoinfo	*vu_fifoinfo;	/* v fifo (VFIFO) */
	} v_un;

	/*
	 * vfs_hash: (mount + inode) -> vnode hash.  The hash value
	 * itself is grouped with other int fields, to avoid padding.
	 */
	LIST_ENTRY(vnode)	v_hashlist;

	/*
	 * VFS_namecache stuff
	 */
	LIST_HEAD(, namecache) v_cache_src;	/* c Cache entries from us */
	TAILQ_HEAD(, namecache) v_cache_dst;	/* c Cache entries to us */
	struct namecache *v_cache_dd;		/* c Cache entry for .. vnode */

	/*
	 * Locking
	 */
	struct	lock v_lock;			/* u (if fs don't have one) */
	struct	mtx v_interlock;		/* lock for "i" things */
	struct	lock *v_vnlock;			/* u pointer to vnode lock */

	/*
	 * The machinery of being a vnode
	 */
	TAILQ_ENTRY(vnode) v_actfreelist;	/* f vnode active/free lists */
	struct bufobj	v_bufobj;		/* * Buffer cache object */

	/*
	 * Hooks for various subsystems and features.
	 */
	struct vpollinfo *v_pollinfo;		/* i Poll events, p for *v_pi */
	struct label *v_label;			/* MAC label for vnode */
	struct lockf *v_lockf;		/* Byte-level advisory lock list */
	struct rangelock v_rl;			/* Byte-range lock */

	/*
	 * clustering stuff
	 */
	daddr_t	v_cstart;			/* v start block of cluster */
	daddr_t	v_lasta;			/* v last allocation  */
	daddr_t	v_lastw;			/* v last write  */
	int	v_clen;				/* v length of cur. cluster */

	u_int	v_holdcnt;			/* I prevents recycling. */
	u_int	v_usecount;			/* I ref count of users */
	u_int	v_iflag;			/* i vnode flags (see below) */
	u_int	v_vflag;			/* v vnode flags */
	int	v_writecount;			/* v ref count of writers */
	u_int	v_hash;
	enum	vtype v_type;			/* u vnode type */
};
```

### Namei Data Structures

```c
/* From /sys/sys/nameidata */

struct componentname {
	/*
	 * Arguments to lookup.
	 */
	u_long	cn_nameiop;	/* namei operation */
	u_int64_t cn_flags;	/* flags to namei */
	struct	thread *cn_thread;/* thread requesting lookup */
	struct	ucred *cn_cred;	/* credentials */
	int	cn_lkflags;	/* Lock flags LK_EXCLUSIVE or LK_SHARED */
	/*
	 * Shared between lookup and commit routines.
	 */
	char	*cn_pnbuf;	/* pathname buffer */
	char	*cn_nameptr;	/* pointer to looked up name */
	long	cn_namelen;	/* length of looked up component */
	long	cn_consume;	/* chars to consume in lookup() */
};

/*
 * Encapsulation of namei parameters.
 */
struct nameidata {
	/*
	 * Arguments to namei/lookup.
	 */
	const	char *ni_dirp;		/* pathname pointer */
	enum	uio_seg ni_segflg;	/* location of pathname */
	cap_rights_t ni_rightsneeded;	/* rights required to look up vnode */
	/*
	 * Arguments to lookup.
	 */
	struct  vnode *ni_startdir;	/* starting directory */
	struct	vnode *ni_rootdir;	/* logical root directory */
	struct	vnode *ni_topdir;	/* logical top directory */
	int	ni_dirfd;		/* starting directory for *at functions */
	int	ni_strictrelative;	/* relative lookup only; no '..' */
	/*
	 * Results: returned from namei
	 */
	struct filecaps ni_filecaps;	/* rights the *at base has */
	/*
	 * Results: returned from/manipulated by lookup
	 */
	struct	vnode *ni_vp;		/* vnode of result */
	struct	vnode *ni_dvp;		/* vnode of intermediate directory */
	/*
	 * Shared between namei and lookup/commit routines.
	 */
	size_t	ni_pathlen;		/* remaining chars in path */
	char	*ni_next;		/* next location in pathname */
	u_int	ni_loopcnt;		/* count of symlinks encountered */
	/*
	 * Lookup parameters: this structure describes the subset of
	 * information from the nameidata structure that is passed
	 * through the VOP interface.
	 */
	struct componentname ni_cnd;
};
```

### Mount Data Structure

```c
/*
 * Structure per mounted filesystem.  Each mounted filesystem has an
 * array of operations and an instance record.  The filesystems are
 * put on a doubly linked list.
 *
 * Lock reference:
 *	m - mountlist_mtx
 *	i - interlock
 *	v - vnode freelist mutex
 *
 * Unmarked fields are considered stable as long as a ref is held.
 *
 */
struct mount {
	struct mtx	mnt_mtx;		/* mount structure interlock */
	int		mnt_gen;		/* struct mount generation */
#define	mnt_startzero	mnt_list
	TAILQ_ENTRY(mount) mnt_list;		/* (m) mount list */
	struct vfsops	*mnt_op;		/* operations on fs */
	struct vfsconf	*mnt_vfc;		/* configuration info */
	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnode	*mnt_syncer;		/* syncer vnode */
	int		mnt_ref;		/* (i) Reference count */
	struct vnodelst	mnt_nvnodelist;		/* (i) list of vnodes */
	int		mnt_nvnodelistsize;	/* (i) # of vnodes */
	struct vnodelst	mnt_activevnodelist;	/* (v) list of active vnodes */
	int		mnt_activevnodelistsize;/* (v) # of active vnodes */
	int		mnt_writeopcount;	/* (i) write syscalls pending */
	int		mnt_kern_flag;		/* (i) kernel only flags */
	uint64_t	mnt_flag;		/* (i) flags shared with user */
	struct vfsoptlist *mnt_opt;		/* current mount options */
	struct vfsoptlist *mnt_optnew;		/* new options passed to fs */
	int		mnt_maxsymlinklen;	/* max size of short symlink */
	struct statfs	mnt_stat;		/* cache of filesystem stats */
	struct ucred	*mnt_cred;		/* credentials of mounter */
	void *		mnt_data;		/* private data */
	time_t		mnt_time;		/* last time written*/
	int		mnt_iosize_max;		/* max size for clusters, etc */
	struct netexport *mnt_export;		/* export list */
	struct label	*mnt_label;		/* MAC label for the fs */
	u_int		mnt_hashseed;		/* Random seed for vfs_hash */
	int		mnt_lockref;		/* (i) Lock reference count */
	int		mnt_secondary_writes;   /* (i) # of secondary writes */
	int		mnt_secondary_accwrites;/* (i) secondary wr. starts */
	struct thread	*mnt_susp_owner;	/* (i) thread owning suspension */
#define	mnt_endzero	mnt_gjprovider
	char		*mnt_gjprovider;	/* gjournal provider name */
	struct lock	mnt_explock;		/* vfs_export walkers lock */
	TAILQ_ENTRY(mount) mnt_upper_link;	/* (m) we in the all uppers */
	TAILQ_HEAD(, mount) mnt_uppers;		/* (m) upper mounts over us*/
};
```

## Code Walkthrough

```c
/*
 * Check permissions, allocate an open file structure, and call the device
 * open routine if any.
 */
#ifndef _SYS_SYSPROTO_H_
struct open_args {
	char	*path;
	int	flags;
	int	mode;
};
#endif
int
sys_open(td, uap)
	struct thread *td;
	register struct open_args /* {
		char *path;
		int flags;
		int mode;
	} */ *uap;
{

	return (kern_openat(td, AT_FDCWD, uap->path, UIO_USERSPACE,
	    uap->flags, uap->mode));
}

int
kern_openat(struct thread *td, int fd, char *path, enum uio_seg pathseg,
    int flags, int mode)
{
	struct proc *p = td->td_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct nameidata nd;
	cap_rights_t rights;
	int cmode, error, indx;

	indx = -1;

	AUDIT_ARG_FFLAGS(flags);
	AUDIT_ARG_MODE(mode);
	/* XXX: audit dirfd */
	cap_rights_init(&rights, CAP_LOOKUP);
	flags_to_rights(flags, &rights);
	/*
	 * Only one of the O_EXEC, O_RDONLY, O_WRONLY and O_RDWR flags
	 * may be specified.
	 *
	 * For these checks to make sense, you must know the bit values.
	 *	O_EXEC    = 0x0040000
	 *	O_RDONLY  = 0x0000000
	 *	O_WRONLY  = 0x0000001
	 *	O_RDWR    = 0x0000002
	 *	O_ACCMODE = 0x0000003
	 *
	 *  Hence, O_EXEC can never be set when flags & O_ACCMODE is true.
	 */
	if (flags & O_EXEC) {
		if (flags & O_ACCMODE)
			return (EINVAL);
	} else if ((flags & O_ACCMODE) == O_ACCMODE) {
		return (EINVAL);
	} else {
		flags = FFLAGS(flags);
	}

	/*
	 * Allocate the file descriptor, but don't install a descriptor yet.
	 */
	error = falloc_noinstall(td, &fp);
	if (error != 0)
		return (error);
	/*
	 * An extra reference on `fp' has been held for us by
	 * falloc_noinstall().
	 */
	/* Set the flags early so the finit in devfs can pick them up. */
	fp->f_flag = flags & FMASK;	/* FMASK is a catchall bitmask */
	/* Obtain file creation flags, file permissions, and clear sticky bit */
	cmode = ((mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;

	/*
	 * NDINIT_ATRIGHTS calls NDINIT_ALL to initialize the namei data
	 * structure.
	 *
	 * McKusick Lecture Notes: pathseg determines whether the path is in 
	 * user/kernel space
	 */
	NDINIT_ATRIGHTS(&nd, LOOKUP, FOLLOW | AUDITVNODE1, pathseg, path, fd,
	    &rights, td);
	td->td_dupfd = -1;		/* XXX check for fdopen */

	/* Opens the vnode specified by the path */
	error = vn_open(&nd, &flags, cmode, fp);

	/*
	 * McKusick lecture Notes: Since errors don't necessarily mean failure
	 * depending on the flags, we have to check certain errors.
	 */
	if (error != 0) {
		/*
		 * If the vn_open replaced the method vector, something
		 * wonderous happened deep below and we just pass it up
		 * pretending we know what we do.
		 */
		/*
		 * According to the freeBSD open.2 man page:
		 *   ENXIO:  The named file is a fifo, no process has it open for
		 *           reading, and the arguments specify it to be opened for
		 *           writing.
		 */
		if (error == ENXIO && fp->f_ops != &badfileops)
			goto success;

		/*
		 * Handle special fdopen() case. bleh.
		 *
		 * Don't do this for relative (capability) lookups; we don't
		 * understand exactly what would happen, and we don't think
		 * that it ever should.
		 */
		if (nd.ni_strictrelative == 0 &&
		    (error == ENODEV || error == ENXIO) &&
		    td->td_dupfd >= 0) {
			error = dupfdopen(td, fdp, td->td_dupfd, flags, error,
			    &indx);
			if (error == 0)
				goto success;
		}

		goto bad;
	}
	td->td_dupfd = 0;
	/* Free namei data structure's pathname buffer (nd.ni_dirp) */
	NDFREE(&nd, NDF_ONLY_PNBUF);
	/*
	 * McKusick Lecture Notes: nd.ni_vp is a ptr to the vnode, whereas
	 * nd.ni_dvp is a ptr to the dir containing the vnode.
	 */
	vp = nd.ni_vp;

	/*
	 * Store the vnode, for any f_type. Typically, the vnode use
	 * count is decremented by direct call to vn_closefile() for
	 * files that switched type in the cdevsw fdopen() method.
	 */
	fp->f_vnode = vp;
	/*
	 * If the file wasn't claimed by devfs bind it to the normal
	 * vnode operations here.
	 */
	if (fp->f_ops == &badfileops) {
		KASSERT(vp->v_type != VFIFO, ("Unexpected fifo."));
		fp->f_seqcount = 1;	/* sequential access count */

		/*
		 * Initializes the files data, flag, type, and fileops pointer to the
		 * arguments passed. Suprisingly it's not inline given its simplicity.
		 */ 
		finit(fp, (flags & FMASK) | (fp->f_flag & FHASLOCK),
		    DTYPE_VNODE, vp, &vnops);
	}

	VOP_UNLOCK(vp, 0);	/* unlock file vnode */
	if (flags & O_TRUNC) {
		/* fo_truncate := fileop_truncate. Def in /sys/file.h */
		error = fo_truncate(fp, 0, td->td_ucred, td);
		if (error != 0)
			goto bad;
	}
success:
	/*
	 * If we haven't already installed the FD (for dupfdopen), do so now.
	 */
	if (indx == -1) {
		struct filecaps *fcaps;

#ifdef CAPABILITIES
		if (nd.ni_strictrelative == 1)
			fcaps = &nd.ni_filecaps;
		else
#endif
			fcaps = NULL;

		/* Installs the vnode into the process's descriptor table */
		error = finstall(td, fp, &indx, flags, fcaps);

		/* On success finstall() consumes fcaps. */
		if (error != 0) {
			filecaps_free(&nd.ni_filecaps);
			goto bad;
		}
	} else {
		filecaps_free(&nd.ni_filecaps);
	}

	/*
	 * Release our private reference, leaving the one associated with
	 * the descriptor table intact.
	 */
	fdrop(fp, td);
	td->td_retval[0] = indx;
	return (0);
bad:
	KASSERT(indx == -1, ("indx=%d, should be -1", indx));
	fdrop(fp, td);
	return (error);
}

int
vn_open(ndp, flagp, cmode, fp)
	struct nameidata *ndp;
	int *flagp, cmode;
	struct file *fp;
{
	struct thread *td = ndp->ni_cnd.cn_thread;

	/* Adds current thread's credential along with the vn_open_flags arg */
	return (vn_open_cred(ndp, flagp, cmode, 0, td->td_ucred, fp));
}

/*
 * Common code for vnode open operations via a name lookup.
 * Lookup the vnode and invoke VOP_CREATE if needed.
 * Check permissions, and call the VOP_OPEN or VOP_CREATE routine.
 * 
 * Note that this does NOT free nameidata for the successful case,
 * due to the NDINIT being done elsewhere.
 */
int
vn_open_cred(struct nameidata *ndp, int *flagp, int cmode, u_int vn_open_flags,
    struct ucred *cred, struct file *fp)
{
	struct vnode *vp;
	struct mount *mp;
	struct thread *td = ndp->ni_cnd.cn_thread;
	struct vattr vat;
	struct vattr *vap = &vat;
	int fmode, error;

restart:	/* McKusick Lecture Notes: Restarting entire routine */
	fmode = *flagp;
	if ((fmode & (O_CREAT | O_EXCL | O_DIRECTORY)) == (O_CREAT |
	    O_EXCL | O_DIRECTORY))
		return (EINVAL);
	else if ((fmode & (O_CREAT | O_DIRECTORY)) == O_CREAT) {/* Creating the file */
		ndp->ni_cnd.cn_nameiop = CREATE;
		/*
		 * Set NOCACHE to avoid flushing the cache when
		 * rolling in many files at once.
		 *
		 * McKusick Lecture Notes: NOCACHE is used for renaming/deleting
		 * files. Makes no sense to cache the file name for these ops.
		 *
		 * Other Flags Meanings:
		 *   ISOPEN     = Return a real vnode
		 *   LOCKPARENT = Lock parent's vnode upon return
		 *   LOCKLEAF   = Lock vnode upon return
		*/
		ndp->ni_cnd.cn_flags = ISOPEN | LOCKPARENT | LOCKLEAF | NOCACHE;
		/* Force following symbolic links for nonexclusive opens */
		if ((fmode & O_EXCL) == 0 && (fmode & O_NOFOLLOW) == 0)
			ndp->ni_cnd.cn_flags |= FOLLOW;
		/* Force auditing for vn_open */
		if (!(vn_open_flags & VN_OPEN_NOAUDIT))
			ndp->ni_cnd.cn_flags |= AUDITVNODE1;
		if (vn_open_flags & VN_OPEN_NOCAPCHECK)
			ndp->ni_cnd.cn_flags |= NOCAPCHECK;

		/* 
		 * McKusick Lecture Notes: Exists because writing on certain storage
		 * media like flash is a lot slower than reading, which can prevent
		 * read operations when too many write operations are queued.
		 * Essentially throttles writes to the filesystem.
		 */
		bwillwrite();

		if ((error = namei(ndp)) != 0)
			return (error);
		if (ndp->ni_vp == NULL) {/* Creating a file: name does not exist */
			VATTR_NULL(vap);
			vap->va_type = VREG;	/* regular file */
			vap->va_mode = cmode;	/* inherit cmode flags */
			if (fmode & O_EXCL)
				vap->va_vaflags |= VA_EXCLUSIVE;

			/*
			 * McKusick Lecture Notes: This vn_start_write will fail the first
			 * time since namei always locks ni_dvp for the CREAT case,  and
			 * will succeed the second time because the body of this if
			 * statement unlocks and frees ni_dvp before restarting the entire
			 * function.
			 */
			if (vn_start_write(ndp->ni_dvp, &mp, V_NOWAIT) != 0) {
				NDFREE(ndp, NDF_ONLY_PNBUF);
				vput(ndp->ni_dvp);	/* unlock and release parent vnode ref */

				/*
				 * McKusick Lecture Notes: This vn_start_write will most likely
				 * sleep and after it wakes up we will immediately restart
				 * the entire function.
				 *
				 * V_XSLEEP is a vn_start_write flag that causes the function
				 * to return immediately after the sleep ends.
				 *
				 * PCATCH is a flag OR'd with prio in tsleep so that signal
				 * checks are done before and after the sleep. This thread will
				 * handle the signal with either ERESTART or EINTR.
				 */
				if ((error = vn_start_write(NULL, &mp,
				    V_XSLEEP | PCATCH)) != 0)
					return (error);
				goto restart;
			}
			/* Make namecache entry if necessary */
			if ((vn_open_flags & VN_OPEN_NAMECACHE) != 0)
				ndp->ni_cnd.cn_flags |= MAKEENTRY;
#ifdef MAC
			error = mac_vnode_check_create(cred, ndp->ni_dvp,
			    &ndp->ni_cnd, vap);
			if (error == 0)
#endif
				/* Call ufs_create to create a new file */
				error = VOP_CREATE(ndp->ni_dvp, &ndp->ni_vp,
						   &ndp->ni_cnd, vap);
			vput(ndp->ni_dvp);	/* release locked vnode ref */
			vn_finished_write(mp);
			if (error) {
				NDFREE(ndp, NDF_ONLY_PNBUF);
				return (error);
			}
			fmode &= ~O_TRUNC;	/* clear trunc flag since we created file */
			vp = ndp->ni_vp;
		} else {	/* Creating a file: name already exists */
			if (ndp->ni_dvp == ndp->ni_vp)	/* name == "." case */
				vrele(ndp->ni_dvp);	/* Releases vnode ref */
			else
				vput(ndp->ni_dvp);	/* Unlock and release vnode ref */
			ndp->ni_dvp = NULL;
			vp = ndp->ni_vp;
			if (fmode & O_EXCL) {/* exclusive open? */
				error = EEXIST;
				goto bad;
			}
			fmode &= ~O_CREAT;
		}
	} else {	/* Looking up a file */
		ndp->ni_cnd.cn_nameiop = LOOKUP;
		/*
		 * We still lock the vnode, but we may or may not follow symbolic links
		 * unlike the creation case.
		 */
		ndp->ni_cnd.cn_flags = ISOPEN |
		    ((fmode & O_NOFOLLOW) ? NOFOLLOW : FOLLOW) | LOCKLEAF;
		/* Share locks for no write files */
		if (!(fmode & FWRITE))
			ndp->ni_cnd.cn_flags |= LOCKSHARED;
		/* Force auditing */
		if (!(vn_open_flags & VN_OPEN_NOAUDIT))
			ndp->ni_cnd.cn_flags |= AUDITVNODE1;
		if (vn_open_flags & VN_OPEN_NOCAPCHECK)
			ndp->ni_cnd.cn_flags |= NOCAPCHECK;
		if ((error = namei(ndp)) != 0)
			return (error);
		vp = ndp->ni_vp;
	}	/* Finished obtaining a vnode */

	/* We must open the vnode to obtain its resources */
	error = vn_open_vnode(vp, fmode, cred, td, fp);

	if (error)
		goto bad;
	*flagp = fmode;
	return (0);
bad:
	NDFREE(ndp, NDF_ONLY_PNBUF);
	vput(vp);			/* unlock and release vnode */
	*flagp = fmode;
	ndp->ni_vp = NULL;	/* return NULL vnode in case of error */
	return (error);
}

/*
 * Common code for vnode open operations once a vnode is located.
 * Check permissions, and call the VOP_OPEN routine.
 */
int
vn_open_vnode(struct vnode *vp, int fmode, struct ucred *cred,
    struct thread *td, struct file *fp)
{
	struct mount *mp;
	accmode_t accmode;
	struct flock lf;
	int error, have_flock, lock_flags, type;

	/* Check file types */
	if (vp->v_type == VLNK)
		return (EMLINK);
	if (vp->v_type == VSOCK)
		return (EOPNOTSUPP);

	/* Check if the file mode does not agree with the file type */
	if (vp->v_type != VDIR && fmode & O_DIRECTORY)
		return (ENOTDIR);

	/* Initialize the vnode access mode */
	accmode = 0;
	if (fmode & (FWRITE | O_TRUNC)) {
		if (vp->v_type == VDIR)
			return (EISDIR);
		accmode |= VWRITE;
	}
	if (fmode & FREAD)
		accmode |= VREAD;
	if (fmode & FEXEC)
		accmode |= VEXEC;
	if ((fmode & O_APPEND) && (fmode & FWRITE))
		accmode |= VAPPEND;
#ifdef MAC
	if (fmode & O_CREAT)
		accmode |= VCREAT;
	if (fmode & O_VERIFY)
		accmode |= VVERIFY;
	error = mac_vnode_check_open(cred, vp, accmode);
	if (error)
		return (error);

	accmode &= ~(VCREAT | VVERIFY);
#endif
	if ((fmode & O_CREAT) == 0) {
		if (accmode & VWRITE) {
			error = vn_writechk(vp);
			if (error)
				return (error);
		}
		if (accmode) {
		        error = VOP_ACCESS(vp, accmode, cred, td);
			if (error)
				return (error);
		}
	}
	/* Obtain an exclusive lock for FIFOs if necessary */
	if (vp->v_type == VFIFO && VOP_ISLOCKED(vp) != LK_EXCLUSIVE)
		vn_lock(vp, LK_UPGRADE | LK_RETRY);

	/* Call ufs_open to open the file */
	if ((error = VOP_OPEN(vp, fmode, cred, td, fp)) != 0)
		return (error);

	if (fmode & (O_EXLOCK | O_SHLOCK)) {
		KASSERT(fp != NULL, ("open with flock requires fp"));
		lock_flags = VOP_ISLOCKED(vp);
		VOP_UNLOCK(vp, 0);
		lf.l_whence = SEEK_SET;	/* offset set to offset bytes */
		lf.l_start = 0;			/* offset bytes set to beg of file */
		lf.l_len = 0;			/* len of new file is 0 */

		/*
		 * We use exclusive locks for writable files and shared locks for
		 * read-only files.
		 */
		if (fmode & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;

		/* Does the file block? */
		if ((fmode & FNONBLOCK) == 0)
			type |= F_WAIT;
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, type);
		have_flock = (error == 0);
		vn_lock(vp, lock_flags | LK_RETRY);
		if (error == 0 && vp->v_iflag & VI_DOOMED)
			error = ENOENT;
		/*
		 * Another thread might have used this vnode as an
		 * executable while the vnode lock was dropped.
		 * Ensure the vnode is still able to be opened for
		 * writing after the lock has been obtained.
		 */
		if (error == 0 && accmode & VWRITE)
			error = vn_writechk(vp);
		if (error) {
			VOP_UNLOCK(vp, 0);
			if (have_flock) {
				lf.l_whence = SEEK_SET;
				lf.l_start = 0;
				lf.l_len = 0;
				lf.l_type = F_UNLCK;
				(void) VOP_ADVLOCK(vp, fp, F_UNLCK, &lf,
				    F_FLOCK);
			}
			vn_start_write(vp, &mp, V_WAIT);
			vn_lock(vp, lock_flags | LK_RETRY);
			(void)VOP_CLOSE(vp, fmode, cred, td);
			vn_finished_write(mp);
			/* Prevent second close from fdrop()->vn_close(). */
			if (fp != NULL)
				fp->f_ops= &badfileops;
			return (error);
		}
		fp->f_flag |= FHASLOCK;
	}
	if (fmode & FWRITE) {
		VOP_ADD_WRITECOUNT(vp, 1);
		CTR3(KTR_VFS, "%s: vp %p v_writecount increased to %d",
		    __func__, vp, vp->v_writecount);
	}
	ASSERT_VOP_LOCKED(vp, "vn_open_vnode");
	return (0);
}
```

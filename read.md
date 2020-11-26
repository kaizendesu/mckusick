# Walkthrough of FreeBSD 11's Read System Call (Excluding Assembly Stubs)

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
sys_read
	kern_readv
		fget_read
		_fget
			fget_unlocked	
		dofileread
			vn_io_fault (fo_read)
				foffset_lock_uio
				doio
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
```

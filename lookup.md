# Walkthrough of FreeBSD 11's Pathname Translation

## Contents

1. Code Flow
2. Reading Checklist
3. Important Data Structures
3. Code Walkthrough

## Code Flow

This section describes the code flow in four level deep tree structure.
Each level of the tree corresponds to a function call with its own
unique stack frame, where the only exception is for one-liner functions
that call a function within a return.

```txt
namei
    namei_handle_root
    lookup
        vfs_cache_lookup
            cache_lookup
            ufs_lookup
            ufs_lookup_ino
                ufs_access
                ufs_accessx
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: vfs_lookup.c
    namei               +-+ (Go over in detail on second pass)
    namei_handle_root   ++-
    lookup              +++

File: vfs_cache.c
    vfs_cache_lookup    ++-

File: vfs_default.c
    vop_stdaccess       ++-

File: ufs_vnops.c
    ufs_access          ---
    ufs_accessx         ++-
    ufs_create          +-- (Watched lecture about this function)
	
File: kern_priv.c
    priv_check_cred     ---

File: ufs_lookup.c
    ufs_lookup          ++-
    ufs_lookup_ino      +++ (Go over in detail on second pass)
    ufs_makedirentry    +-- (Watched lecture about this function)
    ufs_direnter        +-- (Watched lecture about this function)
```

## Important Data Structures

### File Data Structures

```c
/* From /sys/sys/file.h */
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

/* From /sys/ufs/ffs/ffs_vnops.c */

/*
 * McKusick Lecture Notes:
 *     - ffs ops = low-level disk stuff, ufs ops = metadata management
 * 
 *     - Vnode ops cascade between three different levels, where a failure to
 *       locate a particular op at one level results in checking the next level.
 *       To switch to the next level, we follow the reference at default.
 *
 *     - The difference between ffs_vnodeops 1 and 2 are merely the size of
 *       the pointers (32bit vs 64 bit).
 */

/* Global vfs data structures for ufs. */
struct vop_vector ffs_vnodeops2 = {
	.vop_default =		&ufs_vnodeops,
	.vop_fsync =		ffs_fsync,
	.vop_getpages =		vnode_pager_local_getpages,
	.vop_getpages_async =	vnode_pager_local_getpages_async,
	.vop_lock1 =		ffs_lock,
	.vop_read =		ffs_read,
	.vop_reallocblks =	ffs_reallocblks,
	.vop_write =		ffs_write,
	.vop_closeextattr =	ffs_closeextattr,
	.vop_deleteextattr =	ffs_deleteextattr,
	.vop_getextattr =	ffs_getextattr,
	.vop_listextattr =	ffs_listextattr,
	.vop_openextattr =	ffs_openextattr,
	.vop_setextattr =	ffs_setextattr,
	.vop_vptofh =		ffs_vptofh,
};

/* From /sys/ufs/ufs/ufs_vnops.c */

/* Global vfs data structures for ufs. */
struct vop_vector ufs_vnodeops = {
	.vop_default =		&default_vnodeops,
	.vop_fsync =		VOP_PANIC,
	.vop_read =		VOP_PANIC,
	.vop_reallocblks =	VOP_PANIC,
	.vop_write =		VOP_PANIC,
	.vop_accessx =		ufs_accessx,
	.vop_bmap =		ufs_bmap,
	.vop_cachedlookup =	ufs_lookup,
	.vop_close =		ufs_close,
	.vop_create =		ufs_create,
	.vop_getattr =		ufs_getattr,
	.vop_inactive =		ufs_inactive,
	.vop_ioctl =		ufs_ioctl,
	.vop_link =		ufs_link,
	.vop_lookup =		vfs_cache_lookup,
	.vop_markatime =	ufs_markatime,
	.vop_mkdir =		ufs_mkdir,
	.vop_mknod =		ufs_mknod,
	.vop_open =		ufs_open,
	.vop_pathconf =		ufs_pathconf,
	.vop_poll =		vop_stdpoll,
	.vop_print =		ufs_print,
	.vop_readdir =		ufs_readdir,
	.vop_readlink =		ufs_readlink,
	.vop_reclaim =		ufs_reclaim,
	.vop_remove =		ufs_remove,
	.vop_rename =		ufs_rename,
	.vop_rmdir =		ufs_rmdir,
	.vop_setattr =		ufs_setattr,
#ifdef MAC
	.vop_setlabel =		vop_stdsetlabel_ea,
#endif
	.vop_strategy =		ufs_strategy,
	.vop_symlink =		ufs_symlink,
	.vop_whiteout =		ufs_whiteout,
#ifdef UFS_EXTATTR
	.vop_getextattr =	ufs_getextattr,
	.vop_deleteextattr =	ufs_deleteextattr,
	.vop_setextattr =	ufs_setextattr,
#endif
#ifdef UFS_ACL
	.vop_getacl =		ufs_getacl,
	.vop_setacl =		ufs_setacl,
	.vop_aclcheck =		ufs_aclcheck,
#endif
};

/* From /sys/kern/vfs_default.c */

/*
 * This vnode table stores what we want to do if the filesystem doesn't
 * implement a particular VOP.
 *
 * If there is no specific entry here, we will return EOPNOTSUPP.
 *
 * Note that every filesystem has to implement either vop_access
 * or vop_accessx; failing to do so will result in immediate crash
 * due to stack overflow, as vop_stdaccess() calls vop_stdaccessx(),
 * which calls vop_stdaccess() etc.
 */

struct vop_vector default_vnodeops = {
	.vop_default =		NULL,
	.vop_bypass =		VOP_EOPNOTSUPP,

	.vop_access =		vop_stdaccess,
	.vop_accessx =		vop_stdaccessx,
	.vop_advise =		vop_stdadvise,
	.vop_advlock =		vop_stdadvlock,
	.vop_advlockasync =	vop_stdadvlockasync,
	.vop_advlockpurge =	vop_stdadvlockpurge,
	.vop_allocate =		vop_stdallocate,
	.vop_bmap =		vop_stdbmap,
	.vop_close =		VOP_NULL,
	.vop_fsync =		VOP_NULL,
	.vop_getpages =		vop_stdgetpages,
	.vop_getpages_async =	vop_stdgetpages_async,
	.vop_getwritemount = 	vop_stdgetwritemount,
	.vop_inactive =		VOP_NULL,
	.vop_ioctl =		VOP_ENOTTY,
	.vop_kqfilter =		vop_stdkqfilter,
	.vop_islocked =		vop_stdislocked,
	.vop_lock1 =		vop_stdlock,
	.vop_lookup =		vop_nolookup,
	.vop_open =		VOP_NULL,
	.vop_pathconf =		VOP_EINVAL,
	.vop_poll =		vop_nopoll,
	.vop_putpages =		vop_stdputpages,
	.vop_readlink =		VOP_EINVAL,
	.vop_rename =		vop_norename,
	.vop_revoke =		VOP_PANIC,
	.vop_strategy =		vop_nostrategy,
	.vop_unlock =		vop_stdunlock,
	.vop_vptocnp =		vop_stdvptocnp,
	.vop_vptofh =		vop_stdvptofh,
	.vop_unp_bind =		vop_stdunp_bind,
	.vop_unp_connect =	vop_stdunp_connect,
	.vop_unp_detach =	vop_stdunp_detach,
	.vop_is_text =		vop_stdis_text,
	.vop_set_text =		vop_stdset_text,
	.vop_unset_text =	vop_stdunset_text,
	.vop_get_writecount =	vop_stdget_writecount,
	.vop_add_writecount =	vop_stdadd_writecount,
};
```

### Namei Data Structures

```c
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

### Kernel I/O Structures

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

### Mount Structure

```c
/* From /sys/sys/mount.h */

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

### Prison Structure

```c
/* From /sys/sys/jail.h */

struct racct;
struct prison_racct;

/*
 * This structure describes a prison.  It is pointed to by all struct
 * ucreds's of the inmates.  pr_ref keeps track of them and is used to
 * delete the struture when the last inmate is dead.
 *
 * Lock key:
 *   (a) allprison_lock
 *   (p) locked by pr_mtx
 *   (c) set only during creation before the structure is shared, no mutex
 *       required to read
 *   (d) set only during destruction of jail, no mutex needed
 */
struct prison {
	TAILQ_ENTRY(prison) pr_list;			/* (a) all prisons */
	int		 pr_id;				/* (c) prison id */
	int		 pr_ref;			/* (p) refcount */
	int		 pr_uref;			/* (p) user (alive) refcount */
	unsigned	 pr_flags;			/* (p) PR_* flags */
	LIST_HEAD(, prison) pr_children;		/* (a) list of child jails */
	LIST_ENTRY(prison) pr_sibling;			/* (a) next in parent's list */
	struct prison	*pr_parent;			/* (c) containing jail */
	struct mtx	 pr_mtx;
	struct task	 pr_task;			/* (d) destroy task */
	struct osd	 pr_osd;			/* (p) additional data */
	struct cpuset	*pr_cpuset;			/* (p) cpuset */
	struct vnet	*pr_vnet;			/* (c) network stack */
	struct vnode	*pr_root;			/* (c) vnode to rdir */
	int		 pr_ip4s;			/* (p) number of v4 IPs */
	int		 pr_ip6s;			/* (p) number of v6 IPs */
	struct in_addr	*pr_ip4;			/* (p) v4 IPs of jail */
	struct in6_addr	*pr_ip6;			/* (p) v6 IPs of jail */
	struct prison_racct *pr_prison_racct;		/* (c) racct jail proxy */
	void		*pr_sparep[3];
	int		 pr_childcount;			/* (a) number of child jails */
	int		 pr_childmax;			/* (p) maximum child jails */
	unsigned	 pr_allow;			/* (p) PR_ALLOW_* flags */
	int		 pr_securelevel;		/* (p) securelevel */
	int		 pr_enforce_statfs;		/* (p) statfs permission */
	int		 pr_devfs_rsnum;		/* (p) devfs ruleset */
	int		 pr_spare[3];
	int		 pr_osreldate;			/* (c) kern.osreldate value */
	unsigned long	 pr_hostid;			/* (p) jail hostid */
	char		 pr_name[MAXHOSTNAMELEN];	/* (p) admin jail name */
	char		 pr_path[MAXPATHLEN];		/* (c) chroot path */
	char		 pr_hostname[MAXHOSTNAMELEN];	/* (p) jail hostname */
	char		 pr_domainname[MAXHOSTNAMELEN];	/* (p) jail domainname */
	char		 pr_hostuuid[HOSTUUIDLEN];	/* (p) jail hostuuid */
	char		 pr_osrelease[OSRELEASELEN];	/* (c) kern.osrelease value */
};

struct prison_racct {
	LIST_ENTRY(prison_racct) prr_next;
	char		prr_name[MAXHOSTNAMELEN];
	u_int		prr_refcount;
	struct racct	*prr_racct;
};
```

### Inode Structure

```c
/* From /sys/ufs/ufs/inode.h */

/*
 * The inode is used to describe each active (or recently active) file in the
 * UFS filesystem. It is composed of two types of information. The first part
 * is the information that is needed only while the file is active (such as
 * the identity of the file and linkage to speed its lookup). The second part
 * is the permanent meta-data associated with the file which is read in
 * from the permanent dinode from long term storage when the file becomes
 * active, and is put back when the file is no longer being used.
 *
 * An inode may only be changed while holding either the exclusive
 * vnode lock or the shared vnode lock and the vnode interlock. We use
 * the latter only for "read" and "get" operations that require
 * changing i_flag, or a timestamp. This locking protocol allows executing
 * those operations without having to upgrade the vnode lock from shared to
 * exclusive.
 */
struct inode {
	TAILQ_ENTRY(inode) i_nextsnap; /* snapshot file list. */
	struct	vnode  *i_vnode;/* Vnode associated with this inode. */
	struct	ufsmount *i_ump;/* Ufsmount point associated with this inode. */
	u_int32_t i_flag;	/* flags, see below */
	struct cdev *i_dev;	/* Device associated with the inode. */
	ino_t	  i_number;	/* The identity of the inode. */
	int	  i_effnlink;	/* i_nlink when I/O completes */

	struct	 fs *i_fs;	/* Associated filesystem superblock. */
	struct	 dquot *i_dquot[MAXQUOTAS]; /* Dquot structures. */
	/*
	 * Side effects; used during directory lookup.
	 */
	int32_t	  i_count;	/* Size of free slot in directory. */
	doff_t	  i_endoff;	/* End of useful stuff in directory. */
	doff_t	  i_diroff;	/* Offset in dir, where we found last entry. */
	doff_t	  i_offset;	/* Offset of free space in directory. */

	union {
		struct dirhash *dirhash; /* Hashing for large directories. */
		daddr_t *snapblklist;    /* Collect expunged snapshot blocks. */
	} i_un;

	int	i_nextclustercg; /* last cg searched for cluster */

	/*
	 * Data for extended attribute modification.
 	 */
	u_char	  *i_ea_area;	/* Pointer to malloced copy of EA area */
	unsigned  i_ea_len;	/* Length of i_ea_area */
	int	  i_ea_error;	/* First errno in transaction */
	int	  i_ea_refs;	/* Number of users of EA area */

	/*
	 * Copies from the on-disk dinode itself.
	 */
	u_int16_t i_mode;	/* IFMT, permissions; see below. */
	int16_t	  i_nlink;	/* File link count. */
	u_int64_t i_size;	/* File byte count. */
	u_int32_t i_flags;	/* Status flags (chflags). */
	u_int64_t i_gen;	/* Generation number. */
	u_int32_t i_uid;	/* File owner. */
	u_int32_t i_gid;	/* File group. */
	/*
	 * The real copy of the on-disk inode.
	 */
	union {
		struct ufs1_dinode *din1;	/* UFS1 on-disk dinode. */
		struct ufs2_dinode *din2;	/* UFS2 on-disk dinode. */
	} dinode_u;
};
```

### Directory Entry Structure

```c
/* From /sys/ufs/ufs/dir.h */

/*
 * A directory consists of some number of blocks of DIRBLKSIZ
 * bytes, where DIRBLKSIZ is chosen such that it can be transferred
 * to disk in a single atomic operation (e.g. 512 bytes on most machines).
 *
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 *
 * The macro DIRSIZ(fmt, dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(fmt, dp).  All DIRBLKSIZ bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 */
#define	DIRBLKSIZ	DEV_BSIZE
#define	MAXNAMLEN	255

struct	direct {
	u_int32_t d_ino;		/* inode number of entry */
	u_int16_t d_reclen;		/* length of this record */
	u_int8_t  d_type; 		/* file type, see below */
	u_int8_t  d_namlen;		/* length of string in d_name */
	char	  d_name[MAXNAMLEN + 1];/* name with length <= MAXNAMLEN */
};
```

## Code Walkthrough

```c
/*
 * Convert a pathname into a pointer to a locked vnode.
 *
 * The FOLLOW flag is set when symbolic links are to be followed
 * when they occur at the end of the name translation process.
 * Symbolic links are always followed for all other pathname
 * components other than the last.
 *
 * The segflg defines whether the name is to be copied from user
 * space or kernel space.
 *
 * Overall outline of namei:
 *
 *	copy in name
 *	get starting directory
 *	while (!done && !error) {
 *		call lookup to search path.
 *		if symbolic link, massage name in buffer and continue
 *	}
 */
int
namei(struct nameidata *ndp)
{							/*     McKusick Lecture Notes       */
	struct filedesc *fdp;	/* pointer to file descriptor state */
	char *cp;				/* pointer into pathname argument */
	struct vnode *dp;		/* the directory we are searching */
	struct iovec aiov;		/* uio for reading symbolic links */
	struct uio auio;
	int error, linklen, startdir_used;
	struct componentname *cnp = &ndp->ni_cnd;
	struct thread *td = cnp->cn_thread;
	struct proc *p = td->td_proc;

	ndp->ni_cnd.cn_cred = ndp->ni_cnd.cn_thread->td_ucred;
	KASSERT(cnp->cn_cred && p, ("namei: bad cred/proc"));
	KASSERT((cnp->cn_nameiop & (~OPMASK)) == 0,
	    ("namei: nameiop contaminated with flags"));
	KASSERT((cnp->cn_flags & OPMASK) == 0,
	    ("namei: flags contaminated with nameiops"));
	MPASS(ndp->ni_startdir == NULL || ndp->ni_startdir->v_type == VDIR ||
	    ndp->ni_startdir->v_type == VBAD);
	if (!lookup_shared)
		cnp->cn_flags &= ~LOCKSHARED;
	fdp = p->p_fd;

	/* We will set this ourselves if we need it. */
	cnp->cn_flags &= ~TRAILINGSLASH;

	/*
	 * Get a buffer for the name to be translated, and copy the
	 * name into the buffer.
	 *
	 * MAXPATHLEN = 1024 bytes
	 */
	if ((cnp->cn_flags & HASBUF) == 0)
		cnp->cn_pnbuf = uma_zalloc(namei_zone, M_WAITOK);	
	if (ndp->ni_segflg == UIO_SYSSPACE)
			/* Internal kernel call to open */
		error = copystr(ndp->ni_dirp, cnp->cn_pnbuf,
			    MAXPATHLEN, (size_t *)&ndp->ni_pathlen);
	else	/* user process call to open */
		error = copyinstr(ndp->ni_dirp, cnp->cn_pnbuf,
			    MAXPATHLEN, (size_t *)&ndp->ni_pathlen);

	/*
	 * Don't allow empty pathnames.
	 *
	 * McKusick Lecture Notes: This is a POSIX rule
	 */
	if (error == 0 && *cnp->cn_pnbuf == '\0')
		error = ENOENT;

#ifdef CAPABILITY_MODE
	/*
	 * In capability mode, lookups must be "strictly relative" (i.e.
	 * not an absolute path, and not containing '..' components) to
	 * a real file descriptor, not the pseudo-descriptor AT_FDCWD.
	 */
	if (error == 0 && IN_CAPABILITY_MODE(td) &&
	    (cnp->cn_flags & NOCAPCHECK) == 0) {
		ndp->ni_strictrelative = 1;
		if (ndp->ni_dirfd == AT_FDCWD) {
#ifdef KTRACE
			if (KTRPOINT(td, KTR_CAPFAIL))
				ktrcapfail(CAPFAIL_LOOKUP, NULL, NULL);
#endif
			error = ECAPMODE;
		}
	}
#endif
	if (error != 0) {
		namei_cleanup_cnp(cnp);
		ndp->ni_vp = NULL;
		return (error);
	}
	/* Initialize for infinite symbolic link loop detection */
	ndp->ni_loopcnt = 0;
#ifdef KTRACE
	if (KTRPOINT(td, KTR_NAMEI)) {
		KASSERT(cnp->cn_thread == curthread,
		    ("namei not using curthread"));
		ktrnamei(cnp->cn_pnbuf);
	}
#endif
	/*
	 * Get starting point for the translation.
	 */								/* McKusick Lecture Notes */
	FILEDESC_SLOCK(fdp);			/* obtain filedesc mutex */
	ndp->ni_rootdir = fdp->fd_rdir;	/* root dir */
	VREF(ndp->ni_rootdir);			/* incr reference */
	ndp->ni_topdir = fdp->fd_jdir;	/* jail dir */

	/*
	 * If we are auditing the kernel pathname, save the user pathname.
	 */
	if (cnp->cn_flags & AUDITVNODE1)
		AUDIT_ARG_UPATH1(td, ndp->ni_dirfd, cnp->cn_pnbuf);
	if (cnp->cn_flags & AUDITVNODE2)
		AUDIT_ARG_UPATH2(td, ndp->ni_dirfd, cnp->cn_pnbuf);

	startdir_used = 0;
	dp = NULL;
	cnp->cn_nameptr = cnp->cn_pnbuf;
	if (cnp->cn_pnbuf[0] == '/') {	/* absolute pathname? */
		error = namei_handle_root(ndp, &dp);
	} else {	/* relative pathname */
		if (ndp->ni_startdir != NULL) {	/* startdir explicitly set */
			dp = ndp->ni_startdir;
			startdir_used = 1;
		} else if (ndp->ni_dirfd == AT_FDCWD) {	/* start at workign dir */
			dp = fdp->fd_cdir;
			VREF(dp);
		} else {	/* determine startdir from fd capabilities */
			cap_rights_t rights;
			rights = ndp->ni_rightsneeded;
			cap_rights_set(&rights, CAP_LOOKUP);
			if (cnp->cn_flags & AUDITVNODE1)
				AUDIT_ARG_ATFD1(ndp->ni_dirfd);
			if (cnp->cn_flags & AUDITVNODE2)
				AUDIT_ARG_ATFD2(ndp->ni_dirfd);
			/* Obtain vnode rights and starting directory */
			error = fgetvp_rights(td, ndp->ni_dirfd,
			    &rights, &ndp->ni_filecaps, &dp);
			if (error == EINVAL)
				error = ENOTDIR;
#ifdef CAPABILITIES
			/*
			 * If file descriptor doesn't have all rights,
			 * all lookups relative to it must also be
			 * strictly relative.
			 */
			CAP_ALL(&rights);
			if (!cap_rights_contains(&ndp->ni_filecaps.fc_rights,
			    &rights) ||
			    ndp->ni_filecaps.fc_fcntls != CAP_FCNTL_ALL ||
			    ndp->ni_filecaps.fc_nioctls != -1) {
				ndp->ni_strictrelative = 1;
			}
#endif
		}
		if (error == 0 && dp->v_type != VDIR)
			error = ENOTDIR;
	}
	FILEDESC_SUNLOCK(fdp);
	if (ndp->ni_startdir != NULL && !startdir_used)
		vrele(ndp->ni_startdir);
	/*
	 * If there are any errors in obtaining starting directory,
	 * cleanup and return.
	 */
	if (error != 0) {
		if (dp != NULL)
			vrele(dp);
		vrele(ndp->ni_rootdir);
		namei_cleanup_cnp(cnp);
		return (error);
	}
	SDT_PROBE3(vfs, namei, lookup, entry, dp, cnp->cn_pnbuf,
	    cnp->cn_flags);

	for (;;) {
		ndp->ni_startdir = dp;
		/* Lookup the vnode corresponding pathname component */
		error = lookup(ndp);
		if (error != 0) {
			vrele(ndp->ni_rootdir);
			namei_cleanup_cnp(cnp);
			SDT_PROBE2(vfs, namei, lookup, return, error, NULL);
			return (error);
		}
		/*
		 * If not a symbolic link, we're done.
		 *
		 * This is because lookup only returns upon success, error, or 
		 * symbolic link. We checked for error above, so there are only
		 * two possibilities at this if statement.
		 */
		if ((cnp->cn_flags & ISSYMLINK) == 0) {
			vrele(ndp->ni_rootdir);
			/*
			 * McKusick Lecture Notes: SAVENAME is used for O_CREAT where we
			 * need to keep the pathname buffer so we can avoid having to copy 
			 * in the filename from user space.
			 */
			if ((cnp->cn_flags & (SAVENAME | SAVESTART)) == 0) {
				namei_cleanup_cnp(cnp);
			} else
				cnp->cn_flags |= HASBUF;

			SDT_PROBE2(vfs, namei, lookup, return, 0, ndp->ni_vp);
			return (0);
		}
		/*
		 * McKusick Lecture Notes: this if statement prevents infinite looping
		 * between symbolic links.
		 */
		if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			break;
		}
#ifdef MAC
		if ((cnp->cn_flags & NOMACCHECK) == 0) {
			error = mac_vnode_check_readlink(td->td_ucred,
			    ndp->ni_vp);
			if (error != 0)
				break;
		}
#endif
		/*
		 * McKusick Lecture Notes: There are two cases to handle symbolic links:
		 *     Case 1: The symbolic link is at the end of the pathname, so 
		 *             we can just read it into the beginning of the existing
		 *             buffer and reuse it.
		 *
		 *     Case 2: The symbolic link is NOT at the end of the pathname, so
		 *              we create a new buffer to read thesymbolic link and
		 *              in and concatenate the remainder of the path to the
		 *              symbolic link's buffer. We then free the old buffer.
		 */
		if (ndp->ni_pathlen > 1)
			cp = uma_zalloc(namei_zone, M_WAITOK);	/* Case 2 */
		else
			cp = cnp->cn_pnbuf;	/* Case 1 */

		aiov.iov_base = cp;				/* Addr of buffer to read into */
		aiov.iov_len = MAXPATHLEN;		/* Length of buffer to read into */
		auio.uio_iov = &aiov;			/* Add aiov to scatter/gather list */
		auio.uio_iovcnt = 1;			/* Length of scatter/gather list */
		auio.uio_offset = 0;			/* File offset */
		auio.uio_rw = UIO_READ;			/* Read Operation */
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_td = td;
		auio.uio_resid = MAXPATHLEN;	/* Remaining bytes to process */

		/* Uses auio struct to read the symbolic link into the buffer */
		error = VOP_READLINK(ndp->ni_vp, &auio, cnp->cn_cred);
		if (error != 0) {
			if (ndp->ni_pathlen > 1)	/* allocated new buf? */
				uma_zfree(namei_zone, cp);
			break;
		}
		linklen = MAXPATHLEN - auio.uio_resid;
		if (linklen == 0) {	/* symbolic link empty? */
			if (ndp->ni_pathlen > 1)	/* allocated new buf? */
				uma_zfree(namei_zone, cp);
			error = ENOENT;
			break;
		}
		/* Is symbolic link too long? Note that the inequality is inclusive */
		if (linklen + ndp->ni_pathlen >= MAXPATHLEN) {
			if (ndp->ni_pathlen > 1)	/* nonzero pathlen? */
				uma_zfree(namei_zone, cp);
			error = ENAMETOOLONG;
			break;
		}
		/* Copy remaining pathname to the new buffer */
		if (ndp->ni_pathlen > 1) {
			bcopy(ndp->ni_next, cp + linklen, ndp->ni_pathlen);
			uma_zfree(namei_zone, cnp->cn_pnbuf);
			cnp->cn_pnbuf = cp;
		} else	/* Null-terminate */
			cnp->cn_pnbuf[linklen] = '\0';
		ndp->ni_pathlen += linklen;
		vput(ndp->ni_vp);	/* Unlock and release old vnode */
		dp = ndp->ni_dvp;	/* Assign vnode pointed to by symlink */
		/*
		 * Check if root directory should replace current directory.
		 *
		 * Note that this is a recursive call to namei_handle_root.
		 */
		cnp->cn_nameptr = cnp->cn_pnbuf;
		if (*(cnp->cn_nameptr) == '/') {
			vrele(dp);
			error = namei_handle_root(ndp, &dp);
			if (error != 0) {
				vrele(ndp->ni_rootdir);
				namei_cleanup_cnp(cnp);
				return (error);
			}
		}
	}
	/*
	 * McKusick Lecture Notes: if we reach this code there was some kind
	 * of error, so we just clean up and return.
	 */ 
	vrele(ndp->ni_rootdir);
	namei_cleanup_cnp(cnp);
	vput(ndp->ni_vp);
	ndp->ni_vp = NULL;
	vrele(ndp->ni_dvp);
	SDT_PROBE2(vfs, namei, lookup, return, error, NULL);
	return (error);
}

/*
 * Search a pathname.
 * This is a very central and rather complicated routine.
 *
 * The pathname is pointed to by ni_ptr and is of length ni_pathlen.
 * The starting directory is taken from ni_startdir. The pathname is
 * descended until done, or a symbolic link is encountered. The variable
 * ni_more is clear if the path is completed; it is set to one if a
 * symbolic link needing interpretation is encountered.
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it, the parent directory is returned
 * locked. If flag has WANTPARENT or'ed into it, the parent directory is
 * returned unlocked. Otherwise the parent directory is not returned. If
 * the target of the pathname exists and LOCKLEAF is or'ed into the flag
 * the target is returned locked, otherwise it is returned unlocked.
 * When creating or renaming and LOCKPARENT is specified, the target may not
 * be ".".  When deleting and LOCKPARENT is specified, the target may be ".".
 *
 * Overall outline of lookup:
 *
 * dirloop:
 *	identify next component of name at ndp->ni_ptr
 *	handle degenerate case where name is null string
 *	if .. and crossing mount points and on mounted filesys, find parent
 *	call VOP_LOOKUP routine for next component name
 *	    directory vnode returned in ni_dvp, unlocked unless LOCKPARENT set
 *	    component vnode returned in ni_vp (if it exists), locked.
 *	if result vnode is mounted on and crossing mount points,
 *	    find mounted on vnode
 *	if more components of name, do next level at dirloop
 *	return the answer in ni_vp, locked if LOCKLEAF set
 *	    if LOCKPARENT set, return locked parent in ni_dvp
 *	    if WANTPARENT set, return unlocked parent in ni_dvp
 *
 *  McKusick Lecture Notes: VOP_LOOKUP only handles one component at a time
 *  because the filesystems don't know about mount points.
 */
int
lookup(struct nameidata *ndp)
{
	char *cp;				/* pointer into pathname argument */
	struct vnode *dp = 0;	/* the directory we are searching */
	struct vnode *tdp;		/* saved dp */
	struct mount *mp;		/* mount table entry */
	struct prison *pr;
	int docache;			/* == 0 do not cache last component */
	int wantparent;			/* 1 => wantparent or lockparent flag */
	int rdonly;				/* lookup read-only flag bit */
	int error = 0;
	int dpunlocked = 0;		/* dp has already been unlocked */
	struct componentname *cnp = &ndp->ni_cnd;
	int lkflags_save;
	int ni_dvp_unlocked;
	
	/*
	 * Setup: break out flag bits into variables.
	 */
	ni_dvp_unlocked = 0;
	wantparent = cnp->cn_flags & (LOCKPARENT | WANTPARENT);
	KASSERT(cnp->cn_nameiop == LOOKUP || wantparent,
	    ("CREATE, DELETE, RENAME require LOCKPARENT or WANTPARENT."));
	/* We use XOR so that if NOCACHE is 1, docache is 0 */
	docache = (cnp->cn_flags & NOCACHE) ^ NOCACHE;
	/* We don't CACHE for DELETE or RENAME operations */
	if (cnp->cn_nameiop == DELETE ||
	    (wantparent && cnp->cn_nameiop != CREATE &&
	     cnp->cn_nameiop != LOOKUP))
		docache = 0;
	rdonly = cnp->cn_flags & RDONLY;
	/* ISSYMLINK := symbolic link needs interpretation */
	cnp->cn_flags &= ~ISSYMLINK;
	ndp->ni_dvp = NULL;
	/*
	 * We use shared locks until we hit the parent of the last cn then
	 * we adjust based on the requesting flags.
	 */
	if (lookup_shared)
		cnp->cn_lkflags = LK_SHARED;
	else
		cnp->cn_lkflags = LK_EXCLUSIVE;
	dp = ndp->ni_startdir;
	/* Set startdir to NULLVP since we no longer need it */
	ndp->ni_startdir = NULLVP;
	/* Lock starting directory */
	vn_lock(dp,
	    compute_cn_lkflags(dp->v_mount, cnp->cn_lkflags | LK_RETRY,
	    cnp->cn_flags));

dirloop:
	/*
	 * Search a new directory.
	 *
	 * The last component of the filename is left accessible via
	 * cnp->cn_nameptr for callers that need the name. Callers needing
	 * the name set the SAVENAME flag. When done, they assume
	 * responsibility for freeing the pathname buffer.
	 */
	cnp->cn_consume = 0;
	/* Acquire length of component name and ensure its not too long */
	for (cp = cnp->cn_nameptr; *cp != 0 && *cp != '/'; cp++)
		continue;
	cnp->cn_namelen = cp - cnp->cn_nameptr;
	if (cnp->cn_namelen > NAME_MAX) {
		error = ENAMETOOLONG;
		goto bad;
	}
#ifdef NAMEI_DIAGNOSTIC
	{ char c = *cp;
	*cp = '\0';
	printf("{%s}: ", cnp->cn_nameptr);
	*cp = c; }
#endif
	ndp->ni_pathlen -= cnp->cn_namelen;
	ndp->ni_next = cp;

	/*
	 * Replace multiple slashes by a single slash and trailing slashes
	 * by a null.  This must be done before VOP_LOOKUP() because some
	 * fs's don't know about trailing slashes.  Remember if there were
	 * trailing slashes to handle symlinks, existing non-directories
	 * and non-existing files that won't be directories specially later.
	 */
	/* Comment uses 'replace' but in actuality we are just skipping */
	while (*cp == '/' && (cp[1] == '/' || cp[1] == '\0')) {
		cp++;
		ndp->ni_pathlen--;
		if (*cp == '\0') {
			*ndp->ni_next = '\0';
			cnp->cn_flags |= TRAILINGSLASH;
		}
	}
	/* If the while loop encounters '\0', we assign ni_next twice */
	ndp->ni_next = cp;

	/* Set flags for how to process current component */
	cnp->cn_flags |= MAKEENTRY;
	if (*cp == '\0' && docache == 0)
		cnp->cn_flags &= ~MAKEENTRY;
	if (cnp->cn_namelen == 2 &&
	    cnp->cn_nameptr[1] == '.' && cnp->cn_nameptr[0] == '.')
		cnp->cn_flags |= ISDOTDOT;
	else
		cnp->cn_flags &= ~ISDOTDOT;
	if (*ndp->ni_next == 0)
		cnp->cn_flags |= ISLASTCN;
	else
		cnp->cn_flags &= ~ISLASTCN;

	/* We can only delete/rename the last component */
	if ((cnp->cn_flags & ISLASTCN) != 0 &&
	    cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.' &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EINVAL;
		goto bad;
	}

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. like "/." or ".".
	 */
	if (cnp->cn_nameptr[0] == '\0') {
		if (dp->v_type != VDIR) {
			error = ENOTDIR;
			goto bad;
		}
		/* We cannot create, rename, or delete directories */
		if (cnp->cn_nameiop != LOOKUP) {
			error = EISDIR;
			goto bad;
		}
		/* Parent and leaf are the same in degenerate case */
		if (wantparent) {
			ndp->ni_dvp = dp;
			VREF(dp);
		}
		ndp->ni_vp = dp;

		if (cnp->cn_flags & AUDITVNODE1)
			AUDIT_ARG_VNODE1(dp);
		else if (cnp->cn_flags & AUDITVNODE2)
			AUDIT_ARG_VNODE2(dp);

		if (!(cnp->cn_flags & (LOCKPARENT | LOCKLEAF)))
			VOP_UNLOCK(dp, 0);
		/* XXX This should probably move to the top of function. */
		if (cnp->cn_flags & SAVESTART)
			panic("lookup: SAVESTART");
		/*
		 * Since the parent and leaf vnodes are one in the same in the
		 * degenerate case, we already have the vnode we want to lookup.
		 * Hence, we just jump to success.
		 */
		goto success;
	}

	/*
	 * Handle "..": five special cases.
	 * 0. If doing a capability lookup, return ENOTCAPABLE (this is a
	 *    fairly conservative design choice, but it's the only one that we
	 *    are satisfied guarantees the property we're looking for).
	 * 1. Return an error if this is the last component of
	 *    the name and the operation is DELETE or RENAME.
	 * 2. If at root directory (e.g. after chroot)
	 *    or at absolute root directory
	 *    then ignore it so can't get out.
	 * 3. If this vnode is the root of a mounted
	 *    filesystem, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other filesystem.
	 * 4. If the vnode is the top directory of
	 *    the jail or chroot, don't let them out.
	 */
	if (cnp->cn_flags & ISDOTDOT) {
		/* Can't handle ".." with strictrelative set */
		if (ndp->ni_strictrelative != 0) {
#ifdef KTRACE
			if (KTRPOINT(curthread, KTR_CAPFAIL))
				ktrcapfail(CAPFAIL_LOOKUP, NULL, NULL);
#endif
			error = ENOTCAPABLE;
			goto bad;
		}
		/* Case 1: Can't delete or rename a directory */
		if ((cnp->cn_flags & ISLASTCN) != 0 &&
		    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
			error = EINVAL;
			goto bad;
		}
		for (;;) {
			/* This for loop searches for the root directory of the jail */
			for (pr = cnp->cn_cred->cr_prison; pr != NULL;
			     pr = pr->pr_parent)
				if (dp == pr->pr_root)
					break;

			/* Case 2 and Case 4: We do NOT let them past root */ 
			if (dp == ndp->ni_rootdir ||/* Logical root directory */
			    dp == ndp->ni_topdir || /* Logical top directory */
			    dp == rootvnode ||		/* Absolute root directory */
			    pr != NULL ||
			    ((dp->v_vflag & VV_ROOT) != 0 &&
			     (cnp->cn_flags & NOCROSSMOUNT) != 0)) {
				ndp->ni_dvp = dp;
				ndp->ni_vp = dp;
				VREF(dp);
				goto nextname;
			}
			/* If we are not at the root directory, just break */
			if ((dp->v_vflag & VV_ROOT) == 0)
				break;
			if (dp->v_iflag & VI_DOOMED) {	/* forced unmount */
				error = ENOENT;
				goto bad;
			}
			tdp = dp;
			dp = dp->v_mount->mnt_vnodecovered;
			VREF(dp);
			vput(tdp);
			vn_lock(dp,
			    compute_cn_lkflags(dp->v_mount, cnp->cn_lkflags |
			    LK_RETRY, ISDOTDOT));
		}
	}

	/*
	 * We now have a segment name to search for, and a directory to search.
	 */
unionlookup:
#ifdef MAC
	if ((cnp->cn_flags & NOMACCHECK) == 0) {
		error = mac_vnode_check_lookup(cnp->cn_thread->td_ucred, dp,
		    cnp);
		if (error)
			goto bad;
	}
#endif
	ndp->ni_dvp = dp;	/* update parent vnode ptr */
	ndp->ni_vp = NULL;	/* Reset current vnode ptr */
	ASSERT_VOP_LOCKED(dp, "lookup");
	/*
	 * If we have a shared lock we may need to upgrade the lock for the
	 * last operation.
	 */
	if (dp != vp_crossmp &&					/* Not crossing mount point */
	    VOP_ISLOCKED(dp) == LK_SHARED &&	/* Lock is shared */
	    (cnp->cn_flags & ISLASTCN) && (cnp->cn_flags & LOCKPARENT))
		vn_lock(dp, LK_UPGRADE|LK_RETRY);
	/* Check for dead vnode */
	if ((dp->v_iflag & VI_DOOMED) != 0) {
		error = ENOENT;
		goto bad;
	}
	/*
	 * If we're looking up the last component and we need an exclusive
	 * lock, adjust our lkflags.
	 */
	if (needs_exclusive_leaf(dp->v_mount, cnp->cn_flags))
		cnp->cn_lkflags = LK_EXCLUSIVE;
#ifdef NAMEI_DIAGNOSTIC
	vprint("lookup in", dp);
#endif
	lkflags_save = cnp->cn_lkflags;
	cnp->cn_lkflags = compute_cn_lkflags(dp->v_mount, cnp->cn_lkflags,
	    cnp->cn_flags);

	/* Look up the vnode of the pathname component */
	if ((error = VOP_LOOKUP(dp, &ndp->ni_vp, cnp)) != 0) {
		cnp->cn_lkflags = lkflags_save;
		/* Leaf should be empty if there was an error */
		KASSERT(ndp->ni_vp == NULL, ("leaf should be empty"));
#ifdef NAMEI_DIAGNOSTIC
		printf("not found\n");
#endif
		/* We are looking up a vnode in a mounted filesystem */
		if ((error == ENOENT) &&
		    (dp->v_vflag & VV_ROOT) && (dp->v_mount != NULL) &&
		    (dp->v_mount->mnt_flag & MNT_UNION)) {
			tdp = dp;
			dp = dp->v_mount->mnt_vnodecovered;
			VREF(dp);
			vput(tdp);
			vn_lock(dp,
			    compute_cn_lkflags(dp->v_mount, cnp->cn_lkflags |
			    LK_RETRY, cnp->cn_flags));
			goto unionlookup;
		}

		if (error != EJUSTRETURN)
			goto bad;
		/*
		 * At this point, we know we're at the end of the
		 * pathname.  If creating / renaming, we can consider
		 * allowing the file or directory to be created / renamed,
		 * provided we're not on a read-only filesystem.
		 */
		if (rdonly) {
			error = EROFS;
			goto bad;
		}
		/* trailing slash only allowed for directories */
		if ((cnp->cn_flags & TRAILINGSLASH) &&
		    !(cnp->cn_flags & WILLBEDIR)) {
			error = ENOENT;
			goto bad;
		}
		if ((cnp->cn_flags & LOCKPARENT) == 0)
			VOP_UNLOCK(dp, 0);
		/*
		 * We return with ni_vp NULL to indicate that the entry
		 * doesn't currently exist, leaving a pointer to the
		 * (possibly locked) directory vnode in ndp->ni_dvp.
		 */
		if (cnp->cn_flags & SAVESTART) {
			ndp->ni_startdir = ndp->ni_dvp;
			VREF(ndp->ni_startdir);
		}
		goto success;
	} else
		cnp->cn_lkflags = lkflags_save;
#ifdef NAMEI_DIAGNOSTIC
	printf("found\n");
#endif
	/*
	 * Take into account any additional components consumed by
	 * the underlying filesystem.
	 */
	if (cnp->cn_consume > 0) {
		cnp->cn_nameptr += cnp->cn_consume;
		ndp->ni_next += cnp->cn_consume;
		ndp->ni_pathlen -= cnp->cn_consume;
		cnp->cn_consume = 0;
	}

	dp = ndp->ni_vp;

	/*
	 * Check to see if the vnode has been mounted on;
	 * if so find the root of the mounted filesystem.
	 */
	while (dp->v_type == VDIR && (mp = dp->v_mountedhere) &&
	       (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		if (vfs_busy(mp, 0))
			continue;
		/* Lock current vnode */
		vput(dp);
		/* Release/unlock parent vnode */
		if (dp != ndp->ni_dvp)
			vput(ndp->ni_dvp);
		else
			vrele(ndp->ni_dvp);
		/* increment cross mount point vnode */
		vref(vp_crossmp);
		ndp->ni_dvp = vp_crossmp;

		/* Locate root of mounted file system */
		error = VFS_ROOT(mp, compute_cn_lkflags(mp, cnp->cn_lkflags,
		    cnp->cn_flags), &tdp);
		vfs_unbusy(mp);
		if (vn_lock(vp_crossmp, LK_SHARED | LK_NOWAIT))
			panic("vp_crossmp exclusively locked or reclaimed");
		if (error) {
			dpunlocked = 1;
			goto bad2;
		}
		ndp->ni_vp = dp = tdp;
	}

	/*
	 * Check for symbolic link
	 */
	if ((dp->v_type == VLNK) &&
	    ((cnp->cn_flags & FOLLOW) || (cnp->cn_flags & TRAILINGSLASH) ||
	     *ndp->ni_next == '/')) {
		cnp->cn_flags |= ISSYMLINK;
		if (dp->v_iflag & VI_DOOMED) {
			/*
			 * We can't know whether the directory was mounted with
			 * NOSYMFOLLOW, so we can't follow safely.
			 */
			error = ENOENT;
			goto bad2;
		}
		/* Can't follow the link if the NOSYMFOLLOW is set */
		if (dp->v_mount->mnt_flag & MNT_NOSYMFOLLOW) {
			error = EACCES;
			goto bad2;
		}
		/*
		 * Symlink code always expects an unlocked dvp.
		 */
		if (ndp->ni_dvp != ndp->ni_vp) {
			VOP_UNLOCK(ndp->ni_dvp, 0);
			ni_dvp_unlocked = 1;
		}
		goto success;
	}

nextname:
	/*
	 * Not a symbolic link that we will follow.  Continue with the
	 * next component if there is any; otherwise, we're done.
	 */
	KASSERT((cnp->cn_flags & ISLASTCN) || *ndp->ni_next == '/',
	    ("lookup: invalid path state."));
	/* Set nameptr to point at the beginning of next component */
	if (*ndp->ni_next == '/') {
		cnp->cn_nameptr = ndp->ni_next;
		while (*cnp->cn_nameptr == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}
		/* Handle extra reference on the parent directory */
		if (ndp->ni_dvp != dp)
			vput(ndp->ni_dvp);
		else
			vrele(ndp->ni_dvp);
		/* Lookup the next component by restarting the algorithm */
		goto dirloop;
	}
	/*
	 * If we're processing a path with a trailing slash,
	 * check that the end result is a directory.
	 */
	/*
	 * Trailing slashes MUST be directories.
	 *   Ex. /projects/mckusick/advanced/walkthroughs/
	 */
	if ((cnp->cn_flags & TRAILINGSLASH) && dp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad2;
	}
	/*
	 * Disallow directory write attempts on read-only filesystems.
	 */
	if (rdonly &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
		goto bad2;
	}
	/* Set parent directory as start directory */
	if (cnp->cn_flags & SAVESTART) {
		ndp->ni_startdir = ndp->ni_dvp;
		VREF(ndp->ni_startdir);
	}
	if (!wantparent) {
		ni_dvp_unlocked = 2;
		if (ndp->ni_dvp != dp)
			vput(ndp->ni_dvp);
		else
			vrele(ndp->ni_dvp);
	} else if ((cnp->cn_flags & LOCKPARENT) == 0 && ndp->ni_dvp != dp) {
		VOP_UNLOCK(ndp->ni_dvp, 0);
		ni_dvp_unlocked = 1;
	}

	if (cnp->cn_flags & AUDITVNODE1)
		AUDIT_ARG_VNODE1(dp);
	else if (cnp->cn_flags & AUDITVNODE2)
		AUDIT_ARG_VNODE2(dp);

	if ((cnp->cn_flags & LOCKLEAF) == 0)
		VOP_UNLOCK(dp, 0);
success:
	/*
	 * Because of lookup_shared we may have the vnode shared locked, but
	 * the caller may want it to be exclusively locked.
	 */
	if (needs_exclusive_leaf(dp->v_mount, cnp->cn_flags) &&
	    VOP_ISLOCKED(dp) != LK_EXCLUSIVE) {
		vn_lock(dp, LK_UPGRADE | LK_RETRY);
		/* Check if we obtained an exclusive lock on a dead vnode */
		if (dp->v_iflag & VI_DOOMED) { /* VI_DOOMED = vnode being recycled */
			error = ENOENT;
			goto bad2;
		}
	}
	return (0);

bad2:
	if (ni_dvp_unlocked != 2) {
		if (dp != ndp->ni_dvp && !ni_dvp_unlocked)
			vput(ndp->ni_dvp);
		else
			vrele(ndp->ni_dvp);
	}
bad:
	if (!dpunlocked)
		vput(dp);
	ndp->ni_vp = NULL;
	return (error);
}
```

# Walkthrough of FreeBSD 11's System Call Interface

## Contents

1. Code Flow
2. Reading Checklist
3. Important Data Structures
4. Code Walkthrough

## Code Flow

/* I'll do this later */

```txt
SYSCALL
fast_syscall:
amd64_syscall
```

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.


```txt
File: exception.S
	fast_syscall				+++

File: trap.c
	amd64_syscall				+++
	cpu_fetch_syscall_args		++-

File: subr_syscall
	syscallenter				+++
	syscallret					+-+

File: vm_machdep.c
	cpu_set_syscall_retval		++-

File: subr_trap.c
	userret						++-
```

## Important Data Structures

### Segmentation Data Structures

```c
/* From /sys/amd64/include/segments.h */

/*
 * System segment descriptors (128 bit wide)
 */
struct	system_segment_descriptor {
	u_int64_t sd_lolimit:16;	/* segment extent (lsb) */
	u_int64_t sd_lobase:24;		/* segment base address (lsb) */
	u_int64_t sd_type:5;		/* segment type */
	u_int64_t sd_dpl:2;		/* segment descriptor priority level */
	u_int64_t sd_p:1;		/* segment descriptor present */
	u_int64_t sd_hilimit:4;		/* segment extent (msb) */
	u_int64_t sd_xx0:3;		/* unused */
	u_int64_t sd_gran:1;		/* limit granularity (byte/page units)*/
	u_int64_t sd_hibase:40 __packed;/* segment base address  (msb) */
	u_int64_t sd_xx1:8;
	u_int64_t sd_mbz:5;		/* MUST be zero */
	u_int64_t sd_xx2:19;
} __packed;

/*
 * Software definitions are in this convenient format,
 * which are translated into inconvenient segment descriptors
 * when needed to be used by the 386 hardware
 */

struct	soft_segment_descriptor {
	unsigned long ssd_base;		/* segment base address  */
	unsigned long ssd_limit;	/* segment extent */
	unsigned long ssd_type:5;	/* segment type */
	unsigned long ssd_dpl:2;	/* segment descriptor priority level */
	unsigned long ssd_p:1;		/* segment descriptor present */
	unsigned long ssd_long:1;	/* long mode (for %cs) */
	unsigned long ssd_def32:1;	/* default 32 vs 16 bit size */
	unsigned long ssd_gran:1;	/* limit granularity (byte/page units)*/
} __packed;

/*
 * region descriptors, used to load gdt/idt tables before segments yet exist.
 */
struct region_descriptor {
	uint64_t rd_limit:16;		/* segment extent */
	uint64_t rd_base:64 __packed;	/* base address  */
} __packed;
```

### Global Descriptor Table (GDT)

For readability, we use the soft\_segment\_descriptor version of the GDT.

```c
/* From /sys/amd64/amd64/machdep.c */

struct soft_segment_descriptor gdt_segs[] = {
/* GNULL_SEL	0 Null Descriptor */
{	.ssd_base = 0x0,
	.ssd_limit = 0x0,
	.ssd_type = 0,
	.ssd_dpl = 0,
	.ssd_p = 0,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
/* GNULL2_SEL	1 Null Descriptor */
{	.ssd_base = 0x0,
	.ssd_limit = 0x0,
	.ssd_type = 0,
	.ssd_dpl = 0,
	.ssd_p = 0,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
/* GUFS32_SEL	2 32 bit %gs Descriptor for user */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMRWA,
	.ssd_dpl = SEL_UPL,
	.ssd_p = 1,
	.ssd_long = 0,
	.ssd_def32 = 1,
	.ssd_gran = 1		},
/* GUGS32_SEL	3 32 bit %fs Descriptor for user */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMRWA,
	.ssd_dpl = SEL_UPL,
	.ssd_p = 1,
	.ssd_long = 0,
	.ssd_def32 = 1,
	.ssd_gran = 1		},
/* GCODE_SEL	4 Code Descriptor for kernel */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMERA,
	.ssd_dpl = SEL_KPL,
	.ssd_p = 1,
	.ssd_long = 1,
	.ssd_def32 = 0,
	.ssd_gran = 1		},
/* GDATA_SEL	5 Data Descriptor for kernel */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMRWA,
	.ssd_dpl = SEL_KPL,
	.ssd_p = 1,
	.ssd_long = 1,
	.ssd_def32 = 0,
	.ssd_gran = 1		},
/* GUCODE32_SEL	6 32 bit Code Descriptor for user */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMERA,
	.ssd_dpl = SEL_UPL,
	.ssd_p = 1,
	.ssd_long = 0,
	.ssd_def32 = 1,
	.ssd_gran = 1		},
/* GUDATA_SEL	7 32/64 bit Data Descriptor for user */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMRWA,
	.ssd_dpl = SEL_UPL,
	.ssd_p = 1,
	.ssd_long = 0,
	.ssd_def32 = 1,
	.ssd_gran = 1		},
/* GUCODE_SEL	8 64 bit Code Descriptor for user */
{	.ssd_base = 0x0,
	.ssd_limit = 0xfffff,
	.ssd_type = SDT_MEMERA,
	.ssd_dpl = SEL_UPL,
	.ssd_p = 1,
	.ssd_long = 1,
	.ssd_def32 = 0,
	.ssd_gran = 1		},
/* GPROC0_SEL	9 Proc 0 Tss Descriptor */
{	.ssd_base = 0x0,
	.ssd_limit = sizeof(struct amd64tss) + IOPERM_BITMAP_SIZE - 1,
	.ssd_type = SDT_SYSTSS,
	.ssd_dpl = SEL_KPL,
	.ssd_p = 1,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
/* Actually, the TSS is a system descriptor which is double size */
{	.ssd_base = 0x0,
	.ssd_limit = 0x0,
	.ssd_type = 0,
	.ssd_dpl = 0,
	.ssd_p = 0,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
/* GUSERLDT_SEL	11 LDT Descriptor */
{	.ssd_base = 0x0,
	.ssd_limit = 0x0,
	.ssd_type = 0,
	.ssd_dpl = 0,
	.ssd_p = 0,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
/* GUSERLDT_SEL	12 LDT Descriptor, double size */
{	.ssd_base = 0x0,
	.ssd_limit = 0x0,
	.ssd_type = 0,
	.ssd_dpl = 0,
	.ssd_p = 0,
	.ssd_long = 0,
	.ssd_def32 = 0,
	.ssd_gran = 0		},
};
```

### AMD64 Process Control Block

```c
/* From /sys/amd64/include/pcb.h */

struct pcb {
	register_t	pcb_r15;	/* (*) */
	register_t	pcb_r14;	/* (*) */
	register_t	pcb_r13;	/* (*) */
	register_t	pcb_r12;	/* (*) */
	register_t	pcb_rbp;	/* (*) */
	register_t	pcb_rsp;	/* (*) */
	register_t	pcb_rbx;	/* (*) */
	register_t	pcb_rip;	/* (*) */
	register_t	pcb_fsbase;
	register_t	pcb_gsbase;
	register_t	pcb_kgsbase;
	register_t	pcb_cr0;
	register_t	pcb_cr2;
	register_t	pcb_cr3;
	register_t	pcb_cr4;
	register_t	pcb_dr0;
	register_t	pcb_dr1;
	register_t	pcb_dr2;
	register_t	pcb_dr3;
	register_t	pcb_dr6;
	register_t	pcb_dr7;

	struct region_descriptor pcb_gdt;
	struct region_descriptor pcb_idt;
	struct region_descriptor pcb_ldt;
	uint16_t	pcb_tr;

	u_int		pcb_flags;
#define	PCB_FULL_IRET	0x01	/* full iret is required */
#define	PCB_DBREGS	0x02	/* process using debug registers */
#define	PCB_KERNFPU	0x04	/* kernel uses fpu */
#define	PCB_FPUINITDONE	0x08	/* fpu state is initialized */
#define	PCB_USERFPUINITDONE 0x10 /* fpu user state is initialized */
#define	PCB_32BIT	0x40	/* process has 32 bit context (segs etc) */

	uint16_t	pcb_initial_fpucw;

	/* copyin/out fault recovery */
	caddr_t		pcb_onfault;

	uint64_t	pcb_pad0;

	/* local tss, with i/o bitmap; NULL for common */
	struct amd64tss *pcb_tssp;

	/* model specific registers */
	register_t	pcb_efer;
	register_t	pcb_star;
	register_t	pcb_lstar;
	register_t	pcb_cstar;
	register_t	pcb_sfmask;

	struct savefpu	*pcb_save;

	uint64_t	pcb_pad[5];
};
```

### System Call Structures

```c
/* From /sys/sys/sysent.h */

typedef	int	sy_call_t(struct thread *, void *);
typedef	void	(*systrace_probe_func_t)(struct syscall_args *,
		    enum systrace_probe_t, int);
typedef	void	(*systrace_args_func_t)(int, void *, uint64_t *, int *);

struct sysent {	/* system call table */
	int	sy_narg;	/* number of arguments */
	sy_call_t *sy_call;	/* implementing function */
	au_event_t sy_auevent;	/* audit event associated with syscall */
	systrace_args_func_t sy_systrace_args_func;
				/* optional argument conversion function. */
	u_int32_t sy_entry;	/* DTrace entry ID for systrace. */
	u_int32_t sy_return;	/* DTrace return ID for systrace. */
	u_int32_t sy_flags;	/* General flags for system calls. */
	u_int32_t sy_thrcnt;
};

/*
 * The sysentvec structure is contained within the proc structure. This is 
 * so that for each process we can refer to either the freeBSD or Linux sysytem
 * call tables (sysent structs).
 */
struct sysentvec {
	int		sv_size;	/* number of entries */
	struct sysent	*sv_table;	/* pointer to sysent */
	u_int		sv_mask;	/* optional mask to index */
	int		sv_errsize;	/* size of errno translation table */
	int 		*sv_errtbl;	/* errno translation table */
	int		(*sv_transtrap)(int, int);
					/* translate trap-to-signal mapping */
	int		(*sv_fixup)(register_t **, struct image_params *);
					/* stack fixup function */
	void		(*sv_sendsig)(void (*)(int), struct ksiginfo *, struct __sigset *);
			    		/* send signal */
	char 		*sv_sigcode;	/* start of sigtramp code */
	int 		*sv_szsigcode;	/* size of sigtramp code */
	char		*sv_name;	/* name of binary type */
	int		(*sv_coredump)(struct thread *, struct vnode *, off_t, int);
					/* function to dump core, or NULL */
	int		(*sv_imgact_try)(struct image_params *);
	int		sv_minsigstksz;	/* minimum signal stack size */
	int		sv_pagesize;	/* pagesize */
	vm_offset_t	sv_minuser;	/* VM_MIN_ADDRESS */
	vm_offset_t	sv_maxuser;	/* VM_MAXUSER_ADDRESS */
	vm_offset_t	sv_usrstack;	/* USRSTACK */
	vm_offset_t	sv_psstrings;	/* PS_STRINGS */
	int		sv_stackprot;	/* vm protection for stack */
	register_t	*(*sv_copyout_strings)(struct image_params *);
	void		(*sv_setregs)(struct thread *, struct image_params *,
			    u_long);
	void		(*sv_fixlimit)(struct rlimit *, int);
	u_long		*sv_maxssiz;
	u_int		sv_flags;
	void		(*sv_set_syscall_retval)(struct thread *, int);
	int		(*sv_fetch_syscall_args)(struct thread *, struct
			    syscall_args *);
	const char	**sv_syscallnames;
	vm_offset_t	sv_timekeep_base;
	vm_offset_t	sv_shared_page_base;
	vm_offset_t	sv_shared_page_len;
	vm_offset_t	sv_sigcode_base;
	void		*sv_shared_page_obj;
	void		(*sv_schedtail)(struct thread *);
	void		(*sv_thread_detach)(struct thread *);
	int		(*sv_trap)(struct thread *);
};

/* From /sys/amd64/include/proc.h */
struct syscall_args {
	u_int code;
	struct sysent *callp;
	register_t args[8];
	int narg;
};
```

### Signal Information Structures

```c
/* From /sys/sys/signal.h */

typedef	struct __siginfo {
	int	si_signo;		/* signal number */
	int	si_errno;		/* errno association */
	/*
	 * Cause of signal, one of the SI_ macros or signal-specific
	 * values, i.e. one of the FPE_... values for SIGFPE.  This
	 * value is equivalent to the second argument to an old-style
	 * FreeBSD signal handler.
	 */
	int	si_code;		/* signal code */
	__pid_t	si_pid;			/* sending process */
	__uid_t	si_uid;			/* sender's ruid */
	int	si_status;		/* exit value */
	void	*si_addr;		/* faulting instruction */
	union sigval si_value;		/* signal value */
	union	{
		struct {
			int	_trapno;/* machine specific trap code */
		} _fault;
		struct {
			int	_timerid;
			int	_overrun;
		} _timer;
		struct {
			int	_mqd;
		} _mesgq;
		struct {
			long	_band;		/* band event for SIGPOLL */
		} _poll;			/* was this ever used ? */
		struct {
			long	__spare1__;
			int	__spare2__[7];
		} __spare__;
	} _reason;
} siginfo_t;

/* From /sys/sys/signalvar.h */

typedef struct ksiginfo {
	TAILQ_ENTRY(ksiginfo)	ksi_link;
	siginfo_t		ksi_info;
	int			ksi_flags;
	struct sigqueue		*ksi_sigq;
} ksiginfo_t;
```

### Exception Trap Frame

```c
/* From /sys/x86/include/frame.h */

/*
 * Exception/Trap Stack Frame
 *
 * The ordering of this is specifically so that we can take first 6
 * the syscall arguments directly from the beginning of the frame.
 */

struct trapframe {
	register_t	tf_rdi;
	register_t	tf_rsi;
	register_t	tf_rdx;
	register_t	tf_rcx;
	register_t	tf_r8;
	register_t	tf_r9;
	register_t	tf_rax;
	register_t	tf_rbx;
	register_t	tf_rbp;
	register_t	tf_r10;
	register_t	tf_r11;
	register_t	tf_r12;
	register_t	tf_r13;
	register_t	tf_r14;
	register_t	tf_r15;
	uint32_t	tf_trapno;
	uint16_t	tf_fs;
	uint16_t	tf_gs;
	register_t	tf_addr;
	uint32_t	tf_flags;
	uint16_t	tf_es;
	uint16_t	tf_ds;
	/* below portion defined in hardware */
	register_t	tf_err;
	register_t	tf_rip;
	register_t	tf_cs;
	register_t	tf_rflags;
	register_t	tf_rsp;
	register_t	tf_ss;
};
```

## Code Walkthrough

```c
/*
 * Fast syscall entry point.  We enter here with just our new %cs/%ss set,
 * and the new privilige level.  We are still running on the old user stack
 * pointer.  We have to juggle a few things around to find our stack etc.
 * swapgs gives us access to our PCPU space only.
 *
 * We do not support invoking this from a custom %cs or %ss (e.g. using
 * entries from an LDT).
 */

/* #define PCPU(member) %gs:PC_ ## member */

IDTVEC(fast_syscall)
	swapgs	/* switch to kernel %gs; gives access to PCPU space */
	movq	%rsp,PCPU(SCRATCH_RSP)
	movq	PCPU(RSP0),%rsp

	/* Now emulate a trapframe. Make the 8 byte alignment odd for call. */
	subq	$TF_SIZE,%rsp

	/* defer TF_RSP till we have a spare register */
	movq	%r11,TF_RFLAGS(%rsp)	/* SYSCALL stores rflags in %r11 */
	movq	%rcx,TF_RIP(%rsp)		/* SYSCALL stores return address in %rcx */
									/* %rcx original value is in %r10 */
	movq	PCPU(SCRATCH_RSP),%r11	/* %r11 already saved */
	movq	%r11,TF_RSP(%rsp)		/* user stack pointer */
	movw	%fs,TF_FS(%rsp)			/* ... */
	movw	%gs,TF_GS(%rsp)			/* ... */
	movw	%es,TF_ES(%rsp)			/* ... */
	movw	%ds,TF_DS(%rsp)			/* save user data selectors */
	movq	PCPU(CURPCB),%r11		/* %r11 points to current PCB */
	andl	$~PCB_FULL_IRET,PCB_FLAGS(%r11)	/* clear full iret flag */
											/* SYSCALL uses SYSRET, NOT iret */

	sti		/* enable interrupts */

	movq	$KUDSEL,TF_SS(%rsp)	/* KUDSEL is the GUDATA selector with UPL */
	movq	$KUCSEL,TF_CS(%rsp)	/* KUCSEL is the GUCODE selector with UPL */
								/* see Global Descriptor Table Section */

	/*
	 * We store the length of the SYSCALL instruction in tf_err so that we can
	 * restart the system call if necessary by recalculating the appropriate
	 * PC value.
	 */
	movq	$2,TF_ERR(%rsp)
	movq	%rdi,TF_RDI(%rsp)	/* arg 1 */
	movq	%rsi,TF_RSI(%rsp)	/* arg 2 */
	movq	%rdx,TF_RDX(%rsp)	/* arg 3 */
	movq	%r10,TF_RCX(%rsp)	/* arg 4 */
	movq	%r8,TF_R8(%rsp)		/* arg 5 */
	movq	%r9,TF_R9(%rsp)		/* arg 6 */
	movq	%rax,TF_RAX(%rsp)	/* syscall number */
	movq	%rbx,TF_RBX(%rsp)	/* C preserved */
	movq	%rbp,TF_RBP(%rsp)	/* C preserved */
	movq	%r12,TF_R12(%rsp)	/* C preserved */
	movq	%r13,TF_R13(%rsp)	/* C preserved */
	movq	%r14,TF_R14(%rsp)	/* C preserved */
	movq	%r15,TF_R15(%rsp)	/* C preserved */
	movl	$TF_HASSEGS,TF_FLAGS(%rsp)	/* TF_HASSEGS = 1h  */
	cld
	FAKE_MCOUNT(TF_RIP(%rsp))	/* Checks if we are profiling via asm stub */

	/*
	 * Recall the 64 bit C Calling Convention...
	 *    arg1 ==> %rdi
	 *    arg2 ==> %rsi
	 *    arg3 ==> %rdx
	 *    arg4 ==> %rcs
	 *    arg5 ==> %r8
	 *    arg6 ==> %r11
	 *
	 *    Source: https://aaronbloomfield.github.io/pdr/book/x86-64bit-ccc-chapter.pdf
	 */

	movq	PCPU(CURTHREAD),%rdi/* arg1 of amd64_syscall; curthread ptr */
	movq	%rsp,TD_FRAME(%rdi)	/* Set ptr to trap frame in current thread */
	movl	TF_RFLAGS(%rsp),%esi/* arg2 of amd64_syscall; user stk ptr to args */
	andl	$PSL_T,%esi			/* %esi = rFLAGS & trace enable bit */
	call	amd64_syscall
1:	movq	PCPU(CURPCB),%rax
	/* Disable interrupts before testing PCB_FULL_IRET. */
	cli
	testl	$PCB_FULL_IRET,PCB_FLAGS(%rax)
	jnz	3f	/* Don't take jump */
	/* Check for and handle AST's on return to userland. */
	movq	PCPU(CURTHREAD),%rax
	testl	$TDF_ASTPENDING | TDF_NEEDRESCHED,TD_FLAGS(%rax)
	jne	2f	/* Don't take jump */
	/* Restore preserved registers. */
	MEXITCOUNT	/* Handle profiling */
	movq	TF_RDI(%rsp),%rdi	/* bonus; preserve arg 1 */
	movq	TF_RSI(%rsp),%rsi	/* bonus: preserve arg 2 */
	movq	TF_RDX(%rsp),%rdx	/* return value 2 */
	movq	TF_RAX(%rsp),%rax	/* return value 1 */
	movq	TF_RFLAGS(%rsp),%r11	/* original %rflags */
	movq	TF_RIP(%rsp),%rcx	/* original %rip */
	movq	TF_RSP(%rsp),%rsp	/* user stack pointer */
	swapgs	/* switch to user %gs */
	sysretq

/*
 * System call handler for native binaries.  The trap frame is already
 * set up by the assembler trampoline and a pointer to it is saved in
 * td_frame.
 */
void
amd64_syscall(struct thread *td, int traced)	/* %rsi and %rdi */
{
	struct syscall_args sa;	/* structure points to system call table */
	int error;
	ksiginfo_t ksi;

/*
 * Diagnostic checks whether the saved code segment selector has a user
 * priority level.
 */
#ifdef DIAGNOSTIC
	if (ISPL(td->td_frame->tf_cs) != SEL_UPL) {
		panic("syscall");
		/* NOT REACHED */
	}
#endif

	/*
	 * This is where the bulk of the system call is handled. We make a call
	 * into syscallenter to move to MI code asap. Calls to MD code exist
	 * throughout syscallenter on a per need basis.
	 */
	error = syscallenter(td, &sa);

	/*
	 * Traced syscall.
	 */
	if (__predict_false(traced)) {	/* Tell compiler to optimize for traced == 0 */
		/* clear trace bit */
		td->td_frame->tf_rflags &= ~PSL_T;

		/* Clear ksiginfo struct and set KSI_TRAP bit */
		ksiginfo_init_trap(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_code = TRAP_TRACE;

		/* return address */
		ksi.ksi_addr = (void *)td->td_frame->tf_rip;

		/* send signal to current thread */
		trapsignal(td, &ksi);
	}
	/* Kernel assert regarding floating-pointing structures */
	KASSERT(PCB_USER_FPU(td->td_pcb),
	    ("System call %s returing with kernel FPU ctx leaked",
	     syscallname(td->td_proc, sa.code)));

	/* Kernel assert regarding pcb saves */
	KASSERT(td->td_pcb->pcb_save == get_pcb_user_save_td(td),
	    ("System call %s returning with mangled pcb_save",
	     syscallname(td->td_proc, sa.code)));

	/*
	 * This is the code that handles the return value for the sytem call, which
	 * is much more complicated than it was in Unix Version 6.
	 */
	syscallret(td, error, &sa);

	/*
	 * If the user-supplied value of %rip is not a canonical
	 * address, then some CPUs will trigger a ring 0 #GP during
	 * the sysret instruction.  However, the fault handler would
	 * execute in ring 0 with the user's %gs and %rsp which would
	 * not be safe.  Instead, use the full return path which
	 * catches the problem safely.
	 */
	if (td->td_frame->tf_rip >= VM_MAXUSER_ADDRESS)
		set_pcb_flags(td->td_pcb, PCB_FULL_IRET);
}

static inline int
syscallenter(struct thread *td, struct syscall_args *sa)
{
	struct proc *p;
	int error, traced;

	/* Increment pcpu syscall count */
	PCPU_INC(cnt.v_syscall);
	p = td->td_proc;

	/* Reset statclock ticks for syscall profiling */
	td->td_pticks = 0;

	/* cow = copy-on-write; ignore for now */
	if (td->td_cowgen != p->p_cowgen)
		thread_cow_update(td);

	traced = (p->p_flag & P_TRACED) != 0;
	/* If tracing or debugger modified memory/registers */
	if (traced || td->td_dbgflags & TDB_USERWR) {
		PROC_LOCK(p);
		td->td_dbgflags &= ~TDB_USERWR;	/* Clear since restarting syscall */
		if (traced)
			td->td_dbgflags |= TDB_SCE;	/* Means td is performing syscallenter */
		PROC_UNLOCK(p);
	}

	/* perform syscall after fetching args from sa structure */
	error = (p->p_sysent->sv_fetch_syscall_args)(td, sa);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSCALL))
		ktrsyscall(sa->code, sa->narg, sa->args);
#endif

	/* Preprocessor macro dealing with wraparound kernel trace buffer */
	KTR_START4(KTR_SYSC, "syscall", syscallname(p, sa->code),
	    (uintptr_t)td, "pid:%d", td->td_proc->p_pid, "arg0:%p", sa->args[0],
	    "arg1:%p", sa->args[1], "arg2:%p", sa->args[2]);

	if (error == 0) {
		/* Code for witness (deadlock prevention) */
		STOPEVENT(p, S_SCE, sa->narg);
		if (p->p_flag & P_TRACED) {
			PROC_LOCK(p);
			td->td_dbg_sc_code = sa->code;
			td->td_dbg_sc_narg = sa->narg;
			if (p->p_stops & S_PT_SCE)
				ptracestop((td), SIGTRAP);
			PROC_UNLOCK(p);
		}
		if (td->td_dbgflags & TDB_USERWR) {	/* Debug modified mem/regs */
			/*
			 * Reread syscall number and arguments if
			 * debugger modified registers or memory.
			 */
			error = (p->p_sysent->sv_fetch_syscall_args)(td, sa);
			PROC_LOCK(p);
			td->td_dbg_sc_code = sa->code;
			td->td_dbg_sc_narg = sa->narg;
			PROC_UNLOCK(p);
#ifdef KTRACE
			if (KTRPOINT(td, KTR_SYSCALL))
				ktrsyscall(sa->code, sa->narg, sa->args);
#endif
			if (error != 0)
				goto retval;
		}

#ifdef CAPABILITY_MODE
		/*
		 * In capability mode, we only allow access to system calls
		 * flagged with SYF_CAPENABLED.
		 */
		if (IN_CAPABILITY_MODE(td) &&
		    !(sa->callp->sy_flags & SYF_CAPENABLED)) {
			error = ECAPMODE;
			goto retval;
		}
#endif

		/*
		 * Code that ensures every thread terminates before terminating the
		 * entire process.
		 *
		 * McKusick: "Essentially, checking to see whether we are trying to
		 *            exit this process."
		 *
		 * Will come back to this during the scheduling lectures.
		 */
		error = syscall_thread_enter(td, sa->callp);
		if (error != 0)
			goto retval;

#ifdef KDTRACE_HOOKS
		/* Give the syscall:::entry DTrace probe a chance to fire. */
		if (systrace_probe_func != NULL && sa->callp->sy_entry != 0)
			(*systrace_probe_func)(sa, SYSTRACE_ENTRY, 0);
#endif

		/* Audit security subsystem */
		AUDIT_SYSCALL_ENTER(sa->code, td);
		error = (sa->callp->sy_call)(td, sa->args);
		AUDIT_SYSCALL_EXIT(error, td);

		/* Save the latest error return value. */
		if ((td->td_pflags & TDP_NERRNO) == 0)	/* TDP_NERRNO := already set errno */
			td->td_errno = error;

#ifdef KDTRACE_HOOKS
		/* Give the syscall:::return DTrace probe a chance to fire. */
		if (systrace_probe_func != NULL && sa->callp->sy_return != 0)
			(*systrace_probe_func)(sa, SYSTRACE_RETURN,
			    error ? -1 : td->td_retval[0]);
#endif
		/* See syscall_thread_enter */
		syscall_thread_exit(td, sa->callp);
	}
 retval:

	/* Preprocessor macro dealing with wraparound kernel trace buffer */
	KTR_STOP4(KTR_SYSC, "syscall", syscallname(p, sa->code),
	    (uintptr_t)td, "pid:%d", td->td_proc->p_pid, "error:%d", error,
	    "retval0:%#lx", td->td_retval[0], "retval1:%#lx",
	    td->td_retval[1]);

	if (traced) {
		PROC_LOCK(p);
		td->td_dbgflags &= ~TDB_SCE;	/* td finishes syscallenter */
		PROC_UNLOCK(p);
	}

	/* Set the return value of the system call */
	(p->p_sysent->sv_set_syscall_retval)(td, error);
	return (error);
}

static inline void
syscallret(struct thread *td, int error, struct syscall_args *sa)
{
	struct proc *p, *p2;
	int traced;

	/* Kernel assert for fork() */
	KASSERT((td->td_pflags & TDP_FORKING) == 0,
	    ("fork() did not clear TDP_FORKING upon completion"));

	p = td->td_proc;

	/*
	 * Handle reschedule and other end-of-syscall issues.
	 *
	 * More specifically, userret handles any Asynchronous System Traps
	 * triggered during syscall execution, updates td->td_pticks to
	 * account for how many clock cycles we spent in the system call,
	 * adjusts the priority of the process, and finally checks for
	 * several pathological cases like returning with locks held.
	 */
	userret(td, td->td_frame);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSRET)) {
		ktrsysret(sa->code, (td->td_pflags & TDP_NERRNO) == 0 ?
		    error : td->td_errno, td->td_retval[0]);
	}
#endif

	/* Clear "last errno already in td_errno" flag */
	td->td_pflags &= ~TDP_NERRNO;

	if (p->p_flag & P_TRACED) {
		traced = 1;
		PROC_LOCK(p);
		td->td_dbgflags |= TDB_SCX;	/* td is performing syscallret */
		PROC_UNLOCK(p);
	} else
		traced = 0;
	/*
	 * This works because errno is findable through the
	 * register set.  If we ever support an emulation where this
	 * is not the case, this code will need to be revisited.
	 */
	STOPEVENT(p, S_SCX, sa->code);

	if (traced || (td->td_dbgflags & (TDB_EXEC | TDB_FORK)) != 0) {
		PROC_LOCK(p);
		/*
		 * If tracing the execed process, trap to the debugger
		 * so that breakpoints can be set before the program
		 * executes.  If debugger requested tracing of syscall
		 * returns, do it now too.
		 */
		if (traced &&
		    ((td->td_dbgflags & (TDB_FORK | TDB_EXEC)) != 0 ||
		    (p->p_stops & S_PT_SCX) != 0))
			ptracestop(td, SIGTRAP);
		td->td_dbgflags &= ~(TDB_SCX | TDB_EXEC | TDB_FORK);
		PROC_UNLOCK(p);
	}

	if (td->td_pflags & TDP_RFPPWAIT) {	/* handle RFPPWAIT on exit() */
		/*
		 * Preserve synchronization semantics of vfork.  If
		 * waiting for child to exec or exit, fork set
		 * P_PPWAIT on child, and there we sleep on our proc
		 * (in case of exit).
		 *
		 * Do it after the ptracestop() above is finished, to
		 * not block our debugger until child execs or exits
		 * to finish vfork wait.
		 */
		td->td_pflags &= ~TDP_RFPPWAIT;
		p2 = td->td_rfppwait_p;
again:
		PROC_LOCK(p2);
		while (p2->p_flag & P_PPWAIT) {
			PROC_LOCK(p);
			if (thread_suspend_check_needed()) {
				PROC_UNLOCK(p2);
				thread_suspend_check(0);
				PROC_UNLOCK(p);
				goto again;
			} else {
				PROC_UNLOCK(p);
			}
			cv_timedwait(&p2->p_pwait, &p2->p_mtx, hz);
		}
		PROC_UNLOCK(p2);
	}
}
```

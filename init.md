# Walkthrough of FreeBSD 11's Kernel Initialization

## Contents

1. Code Flow
2. Reading Checklist
3. Important Data Structures
4. General Overview (Code)

## Code Flow

```txt
btext
    hammer_time 
        proc_linkup0
        init_param1
        ssdtosd
        ssdtosyssd
        pcpu_init
        dcpu_init
        mutex_init
            init_turnstiles
            mtx_init
                _mtx_init
                    lock_init
        clock_init
            i8254_init
        identify_cpu
        initializecpu
        initializecpucache
        getmemsize
        init_param2
        cninit
        kdb_init
        msgbufinit
        fpuinit
        cpu_probe_amdc1e
    mi_startup
```

## Reading Checklist

The first '+' means that I have read the code or I have a general idea of what it does.
The second '+' means that I have read the code closely and documented it as it relates to bootup.
The third '+' means that I have added it to this markdown document for reference.

```txt
File: locore.S
    hammer_time         ---
    mi_startup          ---

File: machdep.c
    proc_linkup0        ++-
    init_param1         ++-
    ssdtosd             +--
    ssdtosyssd          +--
    wrmsr               ++-
    pcpu_init           ++-
    dpcpu_init          ++-
    mutex_init          ++-
    clock_init          ++-
    identify_cpu        ++- uses CPUID api to obtain CPU information
    initializecpu       ++- initializes cr4 register used CPUID info 
    initializecpucache  ++- sets the CPU cache line size
    getmemsize          ++-
    init_param2         ++-
    cninit              ---
    atpic_reset         --- (?)
    msgbufinit          ---
    fpuinit             ---
    cpu_probe_amdc1e    ---
    x86_init_fdt        --- (?)

File: kern_mutex.c
    init_turnstiles     ++-	initializes 128 turnstile lists and mutexes
    _mtx_init           ++- sets mutex flags and initializes its lock

File: subr_lock.c
    lock_init           ++- sets name and flags, initializes the witness

File: clock.c
    i8254_init          +-- Calls set_i8254_freq for all PCs except PC98
    set_i8254_freq      ++- Sets new_count to 0x10000, maxcount = 0xffff,
                            and clock mode to MODE_PERIODIC (16 bit counter).

File: init_main.c
    mi_startup          ---
```

## Important Data Structures

### Global Descriptor Table (GDT)

```c
/*
 * Software prototypes -- in more palatable form.
 *
 * Keep GUFS32, GUGS32, GUCODE32 and GUDATA at the same
 * slots as corresponding segments for i386 kernel.
 */
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

### PCPU Array

```c
/*
 * This structure maps out the global data that needs to be kept on a
 * per-cpu basis.  The members are accessed via the PCPU_GET/SET/PTR
 * macros defined in <machine/pcpu.h>.  Machine dependent fields are
 * defined in the PCPU_MD_FIELDS macro defined in <machine/pcpu.h>.
 */
struct pcpu {
	struct thread	*pc_curthread;		/* Current thread */
	struct thread	*pc_idlethread;		/* Idle thread */
	struct thread	*pc_fpcurthread;	/* Fp state owner */
	struct thread	*pc_deadthread;		/* Zombie thread or NULL */
	struct pcb	*pc_curpcb;		/* Current pcb */
	uint64_t	pc_switchtime;		/* cpu_ticks() at last csw */
	int		pc_switchticks;		/* `ticks' at last csw */
	u_int		pc_cpuid;		/* This cpu number */
	STAILQ_ENTRY(pcpu) pc_allcpu;
	struct lock_list_entry *pc_spinlocks;
	struct vmmeter	pc_cnt;			/* VM stats counters */
	long		pc_cp_time[CPUSTATES];	/* statclock ticks */
	struct device	*pc_device;
	void		*pc_netisr;		/* netisr SWI cookie */
	int		pc_unused1;		/* unused field */
	int		pc_domain;		/* Memory domain. */
	struct rm_queue	pc_rm_queue;		/* rmlock list of trackers */
	uintptr_t	pc_dynamic;		/* Dynamic per-cpu data area */

	/*
	 * Keep MD fields last, so that CPU-specific variations on a
	 * single architecture don't result in offset variations of
	 * the machine-independent fields of the pcpu.  Even though
	 * the pcpu structure is private to the kernel, some ports
	 * (e.g., lsof, part of gtop) define _KERNEL and include this
	 * header.  While strictly speaking this is wrong, there's no
	 * reason not to keep the offsets of the MI fields constant
	 * if only to make kernel debugging easier.
	 */
	PCPU_MD_FIELDS;
} __aligned(CACHE_LINE_SIZE);

struct pcpu __pcpu[MAXCPU];
```

## General Overview (Code)

```c
u_int64_t
hammer_time(u_int64_t modulep, u_int64_t physfree)
{
	caddr_t kmdp;
	int gsel_tss, x;
	struct pcpu *pc;
	struct nmi_pcpu *np;
	struct xstate_hdr *xhdr;
	u_int64_t msr;
	char *env;
	size_t kstack0_sz;

	/*
 	 * This may be done better later if it gets more high level
 	 * components in it. If so just link td->td_proc here.
	 */
	/*
	 * Initializes static proc0's sig queue and msg queue, allocates its
	 * ksiginfo structure, links proc0 with static thread0, and initializes
	 * the thread's locks, sig queues, and its callout structure.
	 */
	proc_linkup0(&proc0, &thread0);

	kmdp = init_ops.parse_preload_data(modulep);

	/* Init basic tunables, hz etc */
	/*
	 * This function handles system params that do not scale with available
	 * memory size. It uses/modifies the env variables from config files to 
	 * set up hz, ticks, maxswzone, maxbcache, maxtsiz, etc.
	 */
	init_param1();

	thread0.td_kstack = physfree + KERNBASE;			/* kstack base virtual addr */
	thread0.td_kstack_pages = kstack_pages;				/* kstack_pages = 4 */
	kstack0_sz = thread0.td_kstack_pages * PAGE_SIZE;	/* kstack0_sz = 4000h */
	bzero((void *)thread0.td_kstack, kstack0_sz);
	physfree += kstack0_sz;								/* increment physfree by 16KiB */

	/*
	 * make gdt memory segments.
	 * 
	 * We skip GPROC0_SEL and GUSERLDT_SEL because these descriptors are
	 * expanded to 16 bytes in long mode so that the segments can be placed
	 * anywhere in the virtual-addr space.
	 * 
	 * GPROC0_SEL = Available TSS, GUSERLDT_SEL = 32-bit LDT
	 */
	for (x = 0; x < NGDT; x++) {
		if (x != GPROC0_SEL && x != (GPROC0_SEL + 1) &&
		    x != GUSERLDT_SEL && x != (GUSERLDT_SEL) + 1)
			ssdtosd(&gdt_segs[x], &gdt[x]);
	}
	gdt_segs[GPROC0_SEL].ssd_base = (uintptr_t)&common_tss[0];

	/* Set 16-byte descriptor for available TSS */
	ssdtosyssd(&gdt_segs[GPROC0_SEL],
	    (struct system_segment_descriptor *)&gdt[GPROC0_SEL]);

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base =  (long) gdt;
	lgdt(&r_gdt);
	pc = &__pcpu[0];

	/*
	 * wrmsr(u_int msr, uint64_t): Write Model Specific Register.
	 *   Static inline function that writes to the given msr 32 bits at
	 *   a time. Uses inline assembly to do this.
	 */

	wrmsr(MSR_FSBASE, 0);		/* User value */
	wrmsr(MSR_GSBASE, (u_int64_t)pc);	/* MSR_GSBASE contains address of PCPU */
	wrmsr(MSR_KGSBASE, 0);		/* User value while in the kernel */

	/*
	 * Initializes the first cpu in the pcpu array by setting its cpuid, adding
	 * it to the CPU pointer array cpuid_to_pcpu, adding it pc_allcpu's list, 
	 * setting its ACPI ID to 0xffffffff, and initializing its pcpu queue.
	 */
	pcpu_init(pc, 0, sizeof(struct pcpu));

	/* Sets the 0th pcpu's dynamic data area at physfree, copies its default
	 * values from the linker section, and sets its offset in dpcpu_off.
	 */
	dpcpu_init((void *)(physfree + KERNBASE), 0);

	/* Increment physfree to statically allocate the dpcpu set */
	physfree += DPCPU_SIZE;

	/* Fill in pcpu 0 with references to objects we have already set up */
	PCPU_SET(prvspace, pc);			/* prvspace is a self-reference to the cpu */
	PCPU_SET(curthread, &thread0);
	PCPU_SET(tssp, &common_tss[0]);
	PCPU_SET(commontssp, &common_tss[0]);
	PCPU_SET(tss, (struct system_segment_descriptor *)&gdt[GPROC0_SEL]);
	PCPU_SET(ldt, (struct system_segment_descriptor *)&gdt[GUSERLDT_SEL]);
	PCPU_SET(fs32p, &gdt[GUFS32_SEL]);
	PCPU_SET(gs32p, &gdt[GUGS32_SEL]);

	/*
	 * Initialize mutexes.
	 *
	 * icu_lock: in order to allow an interrupt to occur in a critical
	 * 	     section, to set pcpu->ipending (etc...) properly, we
	 *	     must be able to get the icu lock, so it can't be
	 *	     under witness.
	 * 
	 * Initializes turnstiles and major mutexes in the system and
	 * proc0.
	 */
	mutex_init();
	mtx_init(&icu_lock, "icu", NULL, MTX_SPIN | MTX_NOWITNESS);
	mtx_init(&dt_lock, "descriptor tables", NULL, MTX_DEF);

	/* exceptions */
	for (x = 0; x < NIDT; x++)
		setidt(x, &IDTVEC(rsvd), SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_DE, &IDTVEC(div),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_DB, &IDTVEC(dbg),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_NMI, &IDTVEC(nmi),  SDT_SYSIGT, SEL_KPL, 2);
 	setidt(IDT_BP, &IDTVEC(bpt),  SDT_SYSIGT, SEL_UPL, 0);
	setidt(IDT_OF, &IDTVEC(ofl),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_BR, &IDTVEC(bnd),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_UD, &IDTVEC(ill),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_NM, &IDTVEC(dna),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_DF, &IDTVEC(dblfault), SDT_SYSIGT, SEL_KPL, 1);
	setidt(IDT_FPUGP, &IDTVEC(fpusegm),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_TS, &IDTVEC(tss),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_NP, &IDTVEC(missing),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_SS, &IDTVEC(stk),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_GP, &IDTVEC(prot),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_PF, &IDTVEC(page),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_MF, &IDTVEC(fpu),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_AC, &IDTVEC(align), SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_MC, &IDTVEC(mchk),  SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_XF, &IDTVEC(xmm), SDT_SYSIGT, SEL_KPL, 0);
#ifdef KDTRACE_HOOKS
	setidt(IDT_DTRACE_RET, &IDTVEC(dtrace_ret), SDT_SYSIGT, SEL_UPL, 0);
#endif
#ifdef XENHVM
	setidt(IDT_EVTCHN, &IDTVEC(xen_intr_upcall), SDT_SYSIGT, SEL_UPL, 0);
#endif

	r_idt.rd_limit = sizeof(idt0) - 1;
	r_idt.rd_base = (long) idt;
	lidt(&r_idt);

	/*
	 * Initialize the clock before the console so that console
	 * initialization can use DELAY().
	 *
	 * Initializes the clock's mutex and calls i8254_init.
	 */
	clock_init();

	/*
	 * Use vt(4) by default for UEFI boot (during the sc(4)/vt(4)
	 * transition).
	 * Once bootblocks have updated, we can test directly for
	 * efi_systbl != NULL here...
	 */
	if (preload_search_info(kmdp, MODINFO_METADATA | MODINFOMD_EFI_MAP)
	    != NULL)
		vty_set_preferred(VTY_VT);

	identify_cpu();		/* Final stage of CPU initialization */
	initializecpu();	/* Initialize CPU registers */
	initializecpucache();

	/* doublefault stack space, runs on ist1 */
	common_tss[0].tss_ist1 = (long)&dblfault_stack[sizeof(dblfault_stack)];

	/*
	 * NMI stack, runs on ist2.  The pcpu pointer is stored just
	 * above the start of the ist2 stack.
	 */
	np = ((struct nmi_pcpu *) &nmi0_stack[sizeof(nmi0_stack)]) - 1;
	np->np_pcpu = (register_t) pc;
	common_tss[0].tss_ist2 = (long) np;

	/* Set the IO permission bitmap (empty due to tss seg limit) */
	common_tss[0].tss_iobase = sizeof(struct amd64tss) + IOPERM_BITMAP_SIZE;

	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(gsel_tss);

	/* Set up the fast syscall stuff */
	msr = rdmsr(MSR_EFER) | EFER_SCE;
	wrmsr(MSR_EFER, msr);
	wrmsr(MSR_LSTAR, (u_int64_t)IDTVEC(fast_syscall));
	wrmsr(MSR_CSTAR, (u_int64_t)IDTVEC(fast_syscall32));
	msr = ((u_int64_t)GSEL(GCODE_SEL, SEL_KPL) << 32) |
	      ((u_int64_t)GSEL(GUCODE32_SEL, SEL_UPL) << 48);
	wrmsr(MSR_STAR, msr);
	wrmsr(MSR_SF_MASK, PSL_NT|PSL_T|PSL_I|PSL_C|PSL_D);

	getmemsize(kmdp, physfree);

	/*
	 * Sets up system params that scale to the machine's memory size. Hence,
	 * it is called after getmemsize. Like init_param1, it uses/modifies env
	 * variables from config files and ensures that no params would result in
	 * system failure. Sets up params such as maxusers, maxproc, maxfiles, etc.
	 */
	init_param2(physmem);

	/* now running on new page tables, configured,and u/iom is accessible */

	cninit();

#ifdef DEV_ISA
#ifdef DEV_ATPIC
	elcr_probe();
	atpic_startup();
#else
	/* Reset and mask the atpics and leave them shut down. */
	atpic_reset();

	/*
	 * Point the ICU spurious interrupt vectors at the APIC spurious
	 * interrupt handler.
	 */
	setidt(IDT_IO_INTS + 7, IDTVEC(spuriousint), SDT_SYSIGT, SEL_KPL, 0);
	setidt(IDT_IO_INTS + 15, IDTVEC(spuriousint), SDT_SYSIGT, SEL_KPL, 0);
#endif
#else
#error "have you forgotten the isa device?";
#endif

	kdb_init();

#ifdef KDB
	if (boothowto & RB_KDB)
		kdb_enter(KDB_WHY_BOOTFLAGS,
		    "Boot flags requested debugger");
#endif

	msgbufinit(msgbufp, msgbufsize);
	fpuinit();

	/*
	 * Set up thread0 pcb after fpuinit calculated pcb + fpu save
	 * area size.  Zero out the extended state header in fpu save
	 * area.
	 */
	thread0.td_pcb = get_pcb_td(&thread0);
	bzero(get_pcb_user_save_td(&thread0), cpu_max_ext_state_size);
	if (use_xsave) {
		xhdr = (struct xstate_hdr *)(get_pcb_user_save_td(&thread0) +
		    1);
		xhdr->xstate_bv = xsave_mask;
	}
	/* make an initial tss so cpu can get interrupt stack on syscall! */
	common_tss[0].tss_rsp0 = (vm_offset_t)thread0.td_pcb;
	/* Ensure the stack is aligned to 16 bytes */
	common_tss[0].tss_rsp0 &= ~0xFul;
	PCPU_SET(rsp0, common_tss[0].tss_rsp0);
	PCPU_SET(curpcb, thread0.td_pcb);

	/* transfer to user mode */

	_ucodesel = GSEL(GUCODE_SEL, SEL_UPL);
	_udatasel = GSEL(GUDATA_SEL, SEL_UPL);
	_ucode32sel = GSEL(GUCODE32_SEL, SEL_UPL);
	_ufssel = GSEL(GUFS32_SEL, SEL_UPL);
	_ugssel = GSEL(GUGS32_SEL, SEL_UPL);

	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);

	/* setup proc 0's pcb */
	thread0.td_pcb->pcb_flags = 0;
	thread0.td_frame = &proc0_tf;

        env = kern_getenv("kernelname");
	if (env != NULL)
		strlcpy(kernelname, env, sizeof(kernelname));

	cpu_probe_amdc1e();

#ifdef FDT
	x86_init_fdt();
#endif

	/* Location of kernel stack for locore */
	return ((u_int64_t)thread0.td_pcb);
}

/*
 * System startup; initialize the world, create process 0, mount root
 * filesystem, and fork to create init and pagedaemon.  Most of the
 * hard work is done in the lower-level initialization routines including
 * startup(), which does memory initialization and autoconfiguration.
 *
 * This allows simple addition of new kernel subsystems that require
 * boot time initialization.  It also allows substitution of subsystem
 * (for instance, a scheduler, kernel profiler, or VM system) by object
 * module.  Finally, it allows for optional "kernel threads".
 */
void
mi_startup(void)
{

	register struct sysinit **sipp;		/* system initialization*/
	register struct sysinit **xipp;		/* interior loop of sort*/
	register struct sysinit *save;		/* bubble*/

#if defined(VERBOSE_SYSINIT)
	int last;
	int verbose;
#endif

	if (boothowto & RB_VERBOSE)
		bootverbose++;

	if (sysinit == NULL) {
		sysinit = SET_BEGIN(sysinit_set);
		sysinit_end = SET_LIMIT(sysinit_set);
	}

restart:
	/*
	 * Perform a bubble sort of the system initialization objects by
	 * their subsystem (primary key) and order (secondary key).
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {
		for (xipp = sipp + 1; xipp < sysinit_end; xipp++) {
			if ((*sipp)->subsystem < (*xipp)->subsystem ||
			     ((*sipp)->subsystem == (*xipp)->subsystem &&
			      (*sipp)->order <= (*xipp)->order))
				continue;	/* skip*/
			save = *sipp;
			*sipp = *xipp;
			*xipp = save;
		}
	}

#if defined(VERBOSE_SYSINIT)
	last = SI_SUB_COPYRIGHT;
	verbose = 0;
#if !defined(DDB)
	printf("VERBOSE_SYSINIT: DDB not enabled, symbol lookups disabled.\n");
#endif
#endif

	/*
	 * Traverse the (now) ordered list of system initialization tasks.
	 * Perform each task, and continue on to the next task.
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {

		if ((*sipp)->subsystem == SI_SUB_DUMMY)
			continue;	/* skip dummy task(s)*/

		if ((*sipp)->subsystem == SI_SUB_DONE)
			continue;

#if defined(VERBOSE_SYSINIT)
		if ((*sipp)->subsystem > last) {
			verbose = 1;
			last = (*sipp)->subsystem;
			printf("subsystem %x\n", last);
		}
		if (verbose) {
#if defined(DDB)
			const char *func, *data;

			func = symbol_name((vm_offset_t)(*sipp)->func,
			    DB_STGY_PROC);
			data = symbol_name((vm_offset_t)(*sipp)->udata,
			    DB_STGY_ANY);
			if (func != NULL && data != NULL)
				printf("   %s(&%s)... ", func, data);
			else if (func != NULL)
				printf("   %s(%p)... ", func, (*sipp)->udata);
			else
#endif
				printf("   %p(%p)... ", (*sipp)->func,
				    (*sipp)->udata);
		}
#endif

		/* Call function */
		(*((*sipp)->func))((*sipp)->udata);

#if defined(VERBOSE_SYSINIT)
		if (verbose)
			printf("done.\n");
#endif

		/* Check off the one we're just done */
		(*sipp)->subsystem = SI_SUB_DONE;

		/* Check if we've installed more sysinit items via KLD */
		if (newsysinit != NULL) {
			if (sysinit != SET_BEGIN(sysinit_set))
				free(sysinit, M_TEMP);
			sysinit = newsysinit;
			sysinit_end = newsysinit_end;
			newsysinit = NULL;
			newsysinit_end = NULL;
			goto restart;
		}
	}

	mtx_assert(&Giant, MA_OWNED | MA_NOTRECURSED);
	mtx_unlock(&Giant);

	/*
	 * Now hand over this thread to swapper.
	 */
	swapper();
	/* NOTREACHED*/
}

```

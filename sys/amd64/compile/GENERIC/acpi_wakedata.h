#define	wakeup_start	0x0000000000000000
#define	wakeup_sw32	0x000000000000005f
#define	wakeup_32	0x0000000000000070
#define	wakeup_sw64	0x00000000000000a4
#define	wakeup_64	0x00000000000000b0
#define	resume_beep	0x00000000000000ce
#define	reset_video	0x00000000000000cf
#define	bootgdt	0x00000000000000d0
#define	bootcode64	0x00000000000000f0
#define	bootdata64	0x00000000000000f8
#define	bootcode32	0x0000000000000100
#define	bootdata32	0x0000000000000108
#define	bootgdtend	0x0000000000000110
#define	wakeup_pagetables	0x0000000000000110
#define	bootgdtdesc	0x0000000000000114
#define	wakeup_pcb	0x0000000000000120
#define	wakeup_ret	0x0000000000000128
#define	wakeup_gdt	0x0000000000000130
#define	dummy	0x000000000000013a
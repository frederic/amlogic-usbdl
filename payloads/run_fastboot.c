/*
 * run_fastboot.c - S905D3 boot fastboot / New World Cup
 */
#define P_WATCHDOG_RESET (volatile unsigned int *)0xFFD0F0DC
#define P_WATCHDOG_CNTL (volatile unsigned int *)0xffd0f0d0

#define writeb(v, addr)	(*(volatile unsigned char *)(addr) = (v))
#define writew(v, addr)	(*(volatile unsigned short *)(addr) = (v))
#define writel(v, addr) (*((volatile unsigned *)(addr)) = v)
#define readl(addr) (*((volatile unsigned *)(addr)))

#define _clear_icache()    ((void (*)(void))0xffff048c)()
#define uart_print(s)    ((void (*)(unsigned char*))0xffff1038)(s)
#define uart_print_field(a, b, c)    ((void (*)(unsigned char*, unsigned int, unsigned char))0xffff3384)(a, b, c)
#define _reset()    ((void (*)(void))0xFFFF0000)()
#define _udelay(n)    ((void (*)(unsigned int))0xffff3430)(n)
#define _usb_new_world_cup_main()  ((void (*)(void))0xffffb538)()
#define _ffffa5a0(n)  ((void (*)(unsigned long))0xffffa5a0)(n)
#define NEWWORLDCUP  (unsigned char*)0xffffcc1c

static inline void watchdog_reset(void)
{
	*P_WATCHDOG_RESET = 0;
}

static inline void watchdog_disable(void)
{
	*P_WATCHDOG_CNTL &= ~((1<<18)|(1<<25));
}

static inline unsigned int usb_new_world_cup_init(unsigned long param1)
{
    unsigned char bVar1;
    unsigned char bVar2;
    unsigned long lVar3;
    unsigned char *pcVar4;
    unsigned long lVar5;

    lVar3 = 0xfffe0000;
    pcVar4 = (unsigned char*)0xfffe3800;
    lVar5 = 0;
    do {
    bVar2 = *(unsigned char *)(lVar3 + 4 + lVar5);
    bVar1 = bVar2 & 0xf;
    if (bVar1 < 10) {
        pcVar4[1] = bVar1 + 0x30;
    }
    else {
        pcVar4[1] = bVar1 + 0x57;
    }
    bVar2 = bVar2 >> 4;
    if (bVar2 < 10) {
        *pcVar4 = bVar2 + 0x30;
    }
    else {
        *pcVar4 = bVar2 + 0x57;
    }
    lVar5 = lVar5 + 1;
    pcVar4 = pcVar4 + 2;
    *pcVar4 = '\0';
    } while (lVar5 != 0xc);
    writel(0xffffa4f0, 0xfffe3cc0);
    writeb(0x12, 0xfffe3c20);
    writew(0x200, 0xfffe3c22);
    writeb(2, 0xfffe3c24);
    writeb(2, 0xfffe3c25);
    writeb(2, 0xfffe3c2f);
    writeb(3, 0xfffe3c30);
    writel(0xffffcc60, 0xfffe3bf8);
    writel(0xffffcc68, 0xfffe3c08);
    writeb(1, 0xfffe3c21);
    writew(0x1b8e, 0xfffe3c28);
    writeb(1, 0xfffe3c31);
    writel(0xfffe3800, 0xfffe3c18);
    writel(0xfffe3c20, 0xfffe3cb0);
    writel(0xfffe3cc8, 0xfffe3cb8);
    writew(0xc004, 0xfffe3c2a);
    writew(0x409, 0xfffe3cd8);
    writel(0xfffe3bf0, 0xfffe3ce0);
    writel(0xfffe3cd8, 0xfffe3cc8);
    writel(0, 0xfffe3cd0);
    writel(param1, 0xfffe3ca8);
    _ffffa5a0(0xfffe3ca8);
    return 0;
}

static inline void usb_new_world_cup(int param1)
{
    writel(0x0, 0xfffe3978);
    writel(0x0, 0xfffe3970);
    writel((unsigned int)(readl(0xff800008) != 0x4f5244b1), 0xfffe39d4);
    if(param1 == 0)
    {
        writel(5000000, 0xfffe3b7c);
    } else {
        writel(50000, 0xfffe3b7c);
    }
    writel(param1, 0xfffe3b78);
    writel(0, 0xfffe3b80);// _get_time();
    writeb(0x80, 0xfffe3992);
    writeb(2, 0xfffe3993);
    writeb(2, 0xfffe39c3);
    writeb(2, 0xfffe3983);
    writeb(2, 0xfffe39a4);
    writeb(5, 0xfffe3991);
    writeb(5, 0xfffe39c1);
    writew(0x40, 0xfffe39c4);
    writeb(5, 0xfffe3981);
    writeb(0xff, 0xfffe39a5);
    writel(0xfffe39a0, 0xfffe3b48);
    writeb(9, 0xfffe39a0);
    writeb(0x42, 0xfffe39a6);
    writel(0xfffe3990, 0xfffe3b50);
    writel(0xfffe3980, 0xfffe3b58);
    writel(0xffffcc1c, 0xfffe39b8);
    writeb(7, 0xfffe3990);
    writew(0x200, 0xfffe3994);
    writeb(0, 0xfffe3996);
    writeb(7, 0xfffe39c0);
    writeb(0, 0xfffe39c2);
    writeb(0, 0xfffe39c6);
    writeb(7, 0xfffe3980);
    writeb(0, 0xfffe3982);
    writew(0x200, 0xfffe3984);
    writeb(0, 0xfffe3986);
    writeb(4, 0xfffe39a1);
    writeb(0, 0xfffe39a2);
    writeb(0, 0xfffe39a3);
    writeb(3, 0xfffe39a7);
    writew(0x409, 0xfffe3b88);
    writel(0xfffe39b0, 0xfffe3b90);
    writel(0xfffe3b88, 0xfffe3b68);
    usb_new_world_cup_init(0xffffcc2a);
    while(1)
    {
        watchdog_reset();
        _usb_new_world_cup_main();
    }
}

void _start()
{
    _clear_icache();//always first instruction
    watchdog_disable();//todo maybe remove
    usb_new_world_cup(0);//todo find the right param
    _reset();
}
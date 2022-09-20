/*
 * run_fastboot.c - S905D3 boot fastboot / New World Cup
 */
#define P_WATCHDOG_RESET (volatile unsigned int *)0xffd0f0dc
#define _clear_icache() ((void (*)(void))0xffff048c)()
#define _dwc_pcd_irq() ((void (*)(void))0xffff8250)()
#define _reset() ((void (*)(void))0xFFFF0000)()
#define _usb_new_world_cup_main() ((void (*)(void))0xffffb538)()
#define _usb_new_world_cup_init(a) ((void (*)(unsigned long))0xffffa5e4)(a)

#define writeb(v, addr) (*(volatile unsigned char *)(addr) = (v))
#define writew(v, addr) (*(volatile unsigned short *)(addr) = (v))
#define writel(v, addr) (*((volatile unsigned *)(addr)) = v)
#define readl(addr) (*((volatile unsigned *)(addr)))

static inline void watchdog_reset(void)
{
    *P_WATCHDOG_RESET = 0;
}

static inline void usb_new_world_cup(int param1)
{
    writel(0x0, 0xfffe3978);
    writel(0x0, 0xfffe3970);
    writel((unsigned int)(readl(0xff800008) != 0x4f5244b1), 0xfffe39d4);
    if (param1 == 0)
    {
        writel(5000000, 0xfffe3b7c);
    }
    else
    {
        writel(50000, 0xfffe3b7c);
    }
    writel(param1, 0xfffe3b78);
    writel(0, 0xfffe3b80); // _get_time();
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
    _usb_new_world_cup_init(0xffffcc2a);
    while (1)
    {
        watchdog_reset();
        _usb_new_world_cup_main();
    }
}

void _start()
{
    _clear_icache(); //always first instruction
    _dwc_pcd_irq();  //clear USB state
    _dwc_pcd_irq();  //after exploitation

    usb_new_world_cup(0);
    _reset();
}
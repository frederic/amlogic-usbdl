/*
 * memdump_over_usb.c - S905D2 BL2 decrypt & dump tool over USB
 */
#define P_WATCHDOG_RESET (volatile unsigned int *)0xFFD0F0DC
#define _clear_icache() ((void (*)(void))0xffff051c)()
#define _dwc_pcd_irq() ((void (*)(void))0xffff6c94)()
#define read_emmc(a, b, c, d, e) ((void (*)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int))0xffff4618)(a, b, c, d, e)
#define _start_bulk_transfer(pcd) ((void (*)(volatile pcd_struct_t *))0xffff5604)(pcd)
#define cpu_sd_emmc_init(sd_port) ((void (*)(unsigned int))0xffff438c)(sd_port)
#define FUN_ffff454c(sd_port) ((void (*)(unsigned int))0xffff454c)(sd_port)
#define auth_image(a, b, c) ((void (*)(unsigned int, unsigned int, unsigned int))0xffff1ff8)(a, b, c)
#define DLBUF_ADDR 0xfffb0000
#define EMMC_PORT 0xffe07000

typedef struct pcd_struct_s
{
    unsigned int d32[2];
    int ep0state;
    char *buf;
    int length;
    char *bulk_buf;
    int bulk_len;
    int bulk_num;
    int bulk_data_len;
    int xfer_len;
    char bulk_out;
    char bulk_lock;
    unsigned request_config : 1;
    unsigned request_enable : 1;
} pcd_struct_t;

static inline void usb_setup_bulk_in(unsigned char *buf, unsigned int len)
{
    volatile pcd_struct_t *pcd = (pcd_struct_t *)0xfffe3918;
    pcd->bulk_out = 0; // BULK IN
    pcd->bulk_buf = buf;
    pcd->bulk_data_len = len;
    pcd->bulk_len = 0x200;

    _start_bulk_transfer(pcd);
}

static inline void watchdog_reset(void)
{
    *P_WATCHDOG_RESET = 0;
}

void _start()
{
    _clear_icache(); //always first instruction
    _dwc_pcd_irq();  //clear USB state
    _dwc_pcd_irq();  //after exploitation
    cpu_sd_emmc_init(EMMC_PORT);
    FUN_ffff454c(EMMC_PORT);
    read_emmc(EMMC_PORT, 1, DLBUF_ADDR, 0x10000, 0);
    auth_image(DLBUF_ADDR,0x10000,0);//decrypt image
    usb_setup_bulk_in((unsigned char *)DLBUF_ADDR, 0x10000);
    do
    {
        watchdog_reset();
        _dwc_pcd_irq();
    } while (1);
}
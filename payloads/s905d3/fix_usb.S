.text
.global _start

_start:
    mov        x0,#0x48c
    movk       x0,#0xffff, LSL #16
    blr        x0//_clear_icache
    mov        x21,#0x8250
    movk       x21,#0xffff, LSL #16
    blr        x21//_dwc_pcd_irq
    blr        x21//_dwc_pcd_irq
    mov        x0,#0x1000
    movk       x0,#0xfffa, LSL #16
    br         x0
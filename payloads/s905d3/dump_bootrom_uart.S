.text
.global _start

_start:
	ldr w19, _uart_putc
    mov w20, 0x10000 // size
	ldr w21, _addr
    ldr w22, _watchdog_rst
_dump:
    ldrb w0, [x21], #1
    blr x19
    subs x20, x20, #1
    str wzr, [x22]
    bne _dump
	ret

_uart_putc: .dword 0xffff32f0
_addr: .dword 0xFFFF0000
_watchdog_rst: .dword 0xFFD0F0DC

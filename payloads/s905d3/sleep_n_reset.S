.text
.global _start

_start:
	ldr w19, _udelay
    mov w20, 0x10// seconds
	ldr w21, _bl1_addr
    ldr w22, _watchdog_rst
    ldr w23, _one_second
_sleep:
    str wzr, [x22]
    mov w0, w23
    blr x19
    subs x20, x20, #1
    bne _sleep
_reset:
    br x21

_udelay: .dword 0xffff3430
_bl1_addr: .dword 0xFFFF0000
_watchdog_rst: .dword 0xFFD0F0DC
_one_second: .dword 0x000f4240

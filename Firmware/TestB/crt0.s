    .section .vectors, "ax"

    .org 0x0000
    rjmp _start

    ; 35 vettori come ATmega128 (ma tutti indirizzano default_isr)

    .org 0x0002
    rjmp default_isr
    .org 0x0004
    rjmp default_isr
    .org 0x0006
    rjmp default_isr
    .org 0x0008
    rjmp default_isr
    .org 0x000A
    rjmp default_isr
    .org 0x000C
    rjmp default_isr
    .org 0x000E
    rjmp default_isr
    .org 0x0010
    rjmp default_isr
    .org 0x0012
    rjmp default_isr
    .org 0x0014
    rjmp default_isr
    .org 0x0016
    rjmp default_isr
    .org 0x0018
    rjmp default_isr
    .org 0x001A
    rjmp default_isr
    .org 0x001C
    rjmp default_isr
    .org 0x001E
    rjmp default_isr
    .org 0x0020
    rjmp default_isr
    .org 0x0022
    rjmp default_isr
    .org 0x0024
    rjmp default_isr
    .org 0x0026
    rjmp default_isr
    .org 0x0028
    rjmp default_isr
    .org 0x002A
    rjmp default_isr
    .org 0x002C
    rjmp default_isr
    .org 0x002E
    rjmp default_isr
    .org 0x0030
    rjmp default_isr
    .org 0x0032
    rjmp default_isr
    .org 0x0034
    rjmp default_isr
    .org 0x0036
    rjmp default_isr
    .org 0x0038
    rjmp default_isr
    .org 0x003A
    rjmp default_isr
    .org 0x003C
    rjmp default_isr
    .org 0x003E
    rjmp default_isr
    .org 0x0040
    rjmp default_isr
    .org 0x0042
    rjmp default_isr
    .org 0x0044
    rjmp default_isr
    .org 0x0046
    rjmp default_isr

default_isr:
    reti

; -------------------------------------------------------
; Start
; -------------------------------------------------------

    .section .text

_start:
    ; Disabilita interruzioni
    cli

    ; Imposta SP = RAMEND = 0x10FF
    ldi r16, 0x10
    out 0x3E, r16     ; SPH
    ldi r16, 0xFF
    out 0x3D, r16     ; SPL

    ; Salta al main
    rjmp main

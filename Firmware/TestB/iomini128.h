#ifndef _IOMINI128_H_
#define _IOMINI128_H_

/* Base RAM */
#define RAMSTART   0x0100
#define RAMEND     0x10FF   /* 4 KB RAM */
#define FLASHEND   0x3FFF   /* 16 KB Flash */
#define SPM_PAGESIZE 256

/* Registri I/O dell’ATmega128 – tutti mantenuti */
#define SPL   _SFR_IO8(0x3D)
#define SPH   _SFR_IO8(0x3E)
#define SREG  _SFR_IO8(0x3F)

/* UDR, UCSRA, UCSRB, UBRR... */
#include <avr/iom128.h>  /* eredita tutto il resto */

#endif

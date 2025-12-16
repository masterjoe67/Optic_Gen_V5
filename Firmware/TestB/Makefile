SHELL=/bin/sh

CC=avr-gcc
AS=avr-as
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
PYTHON=python3

MCU=atmega128

CFLAGS = -mmcu=$(MCU) -Os -ffreestanding -fno-builtin \
         -nostdlib -nostartfiles -nodefaultlibs

LDFLAGS = -T mini_128.ld -nostdlib -nostartfiles -nodefaultlibs

all: firmware.elf firmware.bin firmware.lst firmware.mif

firmware.elf: crt0.o main.o
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.o main.o -o firmware.elf

crt0.o: crt0.s
	$(AS) -mmcu=$(MCU) crt0.s -o crt0.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary firmware.elf firmware.bin

firmware.lst: firmware.elf
	$(OBJDUMP) -d -S firmware.elf > firmware.lst

firmware.mif: firmware.bin
	$(PYTHON) bin2mif.py firmware.bin firmware.mif 16 16384

clean:
	rm -f *.o *.elf *.bin *.lst *.mif

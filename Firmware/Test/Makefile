# -------------------------------------------------------------------
# Configurazione
# -------------------------------------------------------------------
ARCH       = atmega128
#ARCH_LIB   = crt0.o
CC        = avr-gcc
AS        = avr-as
OBJCOPY   = avr-objcopy
OBJDUMP   = avr-objdump

SRC  = $(wildcard *.c)
ELF  = firmware.elf
HEX  = firmware.hex
BIN  = firmware.bin
MIF  = firmware.mif
LST  = firmware.lst

#AFLAGS  = -mmcu=$(ARCH)
CFLAGS  = -mmcu=$(ARCH) -funsigned-char -funsigned-bitfields \
          -fpack-struct -fshort-enums -Wall -Wstrict-prototypes \
          -Wno-unused-function -Os
LDFLAGS = -mmcu=$(ARCH)

PMEM_DEPTH=12
PMEM_WORDS=$(shell powershell -Command "$((1<<$(PMEM_DEPTH)))")

# -------------------------------------------------------------------
# Build
# -------------------------------------------------------------------
#all: $(HEX) $(MIF) $(LST)

#crt0.o: crt0.S
#	$(AS) $(AFLAGS) -o crt0.o crt0.S

#$(ELF): crt0.o $(SRC)
#	$(CC) $(CFLAGS) crt0.o $(SRC) -o $(ELF)

#$(HEX): $(ELF)
#	$(OBJCOPY) -O ihex $(ELF) $(HEX)

#$(BIN): $(ELF)
#	$(OBJCOPY) -O binary $(ELF) $(BIN)

all: $(HEX) $(MIF) $(LST)

$(ELF): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(ELF)

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $(ELF) $(HEX)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(BIN)
	
# -------------------------------------------------------------------
# Generazione MIF
# -------------------------------------------------------------------
$(MIF): $(BIN)
	python3 bin2mif.py $(BIN) $(MIF) 16 16384

# -------------------------------------------------------------------
# Listing
# -------------------------------------------------------------------
$(LST): $(ELF)
	$(OBJDUMP) -d $(ELF) > $(LST)

# -------------------------------------------------------------------
# Clean
# -------------------------------------------------------------------
clean:
	rm -f $(ELF) $(HEX) $(BIN) $(MIF) $(LST) crt0.o

.PHONY: all clean

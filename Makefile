PRG            := leds
OBJ            := leds.o
MCU_TARGET     := atmega328p
OPTIMIZE       := -O2

DEFS           :=
LIBS           :=

# You should not have to change anything below here.

CC             := avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        := -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       := -Wl,-Map,$(PRG).map

OBJCOPY        := avr-objcopy
OBJDUMP        := avr-objdump

all: $(PRG).elf lst

program: all
	avrdude -c buspirate -P /dev/ttyUSB0 -p m328p -U flash:w:$(PRG).elf:e

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# dependency:
#leds.o: leds.c

clean:
	rm -rf *.o *.elf *.eps *.png *.pdf *.bak *.lst *.map $(EXTRA_CLEAN_FILES) *~ *.hex *.bin *.srec

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

.PHONY: clean lst all program


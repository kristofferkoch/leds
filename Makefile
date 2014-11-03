PRG            := leds
OBJ            := leds.o color.o
BOOTLOADER_OBJ := bootloader.o i2c.o
MCU_TARGET     := atmega328p
OPTIMIZE       := -Os -flto

DEFS           :=
LIBS           :=

# You should not have to change anything below here.

CC             := avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        := -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       := -Wl,-Map,$(PRG).map -fwhole-program

OBJCOPY        := avr-objcopy
OBJDUMP        := avr-objdump

all: $(PRG).elf lst size

program: $(PRG).hex all
	avrdude -c buspirate -P /dev/ttyUSB0 -p m328p -U flash:w:$(PRG).hex:i

power:
	echo -e "m\n9\nW" > /dev/ttyUSB0
off:
	echo -e "m\n9\nw" > /dev/ttyUSB0

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
bootloader.elf: $(BOOTLOADER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
size:
	avr-size *.o *.elf

# dependency:
#leds.o: leds.c

clean:
	rm -rf *.o *.elf *.eps *.png *.pdf *.bak *.lst *.map $(EXTRA_CLEAN_FILES) *~ *.hex *.bin *.srec

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

.PHONY: clean lst all program

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

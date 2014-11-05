#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>

//#define offsetof(st, m) __builtin_offsetof(st, m)

typedef struct {
    unsigned overflow:1;
    unsigned busy:1;
} status_t;

typedef struct {
    status_t status;
    uint16_t id;
    uint16_t version;
    // All registers after here are read/write
    uint16_t addr;
    uint8_t cmd;
    uint8_t data; // Non-incrementing. Increments addr instead.
    uint16_t length;
    uint16_t result;
} memory_map_t;

memory_map_t memory_map = {.id = 0xabba,
			   .version = 0x0001};

typedef enum {
    CMD_NOP = 0x00,
    CMD_RESET = 0x01,
    CMD_ERASE = 0x02,
    CMD_WRITE = 0x03,
    CMD_CRC = 0x04,
} command_t;

volatile command_t command;
volatile uint8_t have_command = 0;

static void isr_handle_command(command_t cmd) {
    if (memory_map.status.busy)
	return;
    memory_map.status.busy = 1;
    command = cmd;
    have_command = 1;
}

static void write(uint8_t *addr, uint8_t data) {
    uint8_t *bytes = (uint8_t *)&memory_map;
    if (*addr >= sizeof(memory_map)) {
	memory_map.status.overflow = 1;
	return;
    }
    if (*addr < offsetof(memory_map_t, cmd)) {
	return; // Read only
    }
    switch (*addr) {
    case  offsetof(memory_map_t, data):
	// Write to flash buffer
	if (memory_map.addr & 1) {
	    uint16_t word = ((uint16_t)data << 8) | memory_map.data;
	    boot_page_fill_safe(memory_map.addr, word);
	}
	else {
	    memory_map.data = data;
	}
	memory_map.addr += 1;
	return; // No increment
    case offsetof(memory_map_t, cmd):
	isr_handle_command(data);
	break;
    }
    bytes[*addr] = data;
    *addr += 1;
}

static uint8_t read(uint8_t *addr) {
    uint8_t *bytes = (uint8_t *)&memory_map;
    if (*addr >= sizeof(memory_map)) {
	memory_map.status.overflow = 1;
	return 0xff;
    }
    uint8_t ret = bytes[*addr];
    switch (*addr) {
    case offsetof(memory_map_t, status):
	memory_map.status.overflow = 0;
	break;
    case offsetof(memory_map_t, data):
	// Read from flash
	ret = pgm_read_byte(memory_map.addr);
	memory_map.addr += 1;
	return ret; 
    }
    *addr += 1;
    return ret;
}


ISR(TWI_vect) {
    const uint8_t COMMON_TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
    uint8_t twsr = TW_STATUS;
    static uint8_t reg_addr;
    
    switch (twsr) {
    case TW_SR_SLA_ACK: // 0x60
	// Reset address
	reg_addr = 0xff;
	break;
    case TW_SR_DATA_ACK: // 0x80
	// Receive data
	if (reg_addr == 0xff) {
	    reg_addr = TWDR;
	    break;
	}
	write(&reg_addr, TWDR);
	break;
    case TW_ST_DATA_ACK: // 0xB8
	// Transmit data
	TWDR = read(&reg_addr);
	break;
    case TW_BUS_ERROR: // 0x00
	TWCR = COMMON_TWCR | (1 << TWSTO) | (1 << TWEA); // Clear error
	return;
    case TW_NO_INFO: // 0xF8
	return; // Do nothing
    default:
	// Something unexpected or something we dont care about. Try to just acknowledge.
	break;
    }
    TWCR = COMMON_TWCR | (1 << TWEA);
}


static void perform_crc() {
    uint16_t i;
    uint16_t crc = 0xffff;
    for (i = memory_map.addr; i < memory_map.length; i++) {
	crc = _crc16_update(crc, pgm_read_byte(i));
    }
    memory_map.result = crc;
}

static void execute_command(command_t cmd) {
    switch (cmd) {
    case CMD_RESET:
	cli();
	MCUCR = (1 << IVCE);
	MCUCR = 0;
	asm volatile("jmp 0x00");
    case CMD_ERASE:
	cli();
	boot_page_erase_safe(memory_map.addr);
	sei();
	boot_spm_busy_wait();
	break;
    case CMD_WRITE:
	cli();
	boot_page_write_safe(memory_map.addr);
	sei();
	boot_spm_busy_wait();
	break;
    case CMD_CRC:
	perform_crc();
	break;
    case CMD_NOP:
    default:
	break;
    }
}

int main(void) {
    
    TWCR = 0;
    TWSR = 0;
    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
    TWAR = 72;//I2C_SLAVE_ADDR;
    TWAMR = 0;

    MCUCR = (1<<IVCE);
    MCUCR = (1<<IVSEL);
    sei();
    
    volatile memory_map_t *map = &memory_map;
    for (;;) {
	if (have_command) {
	    command_t cmd = command;
	    execute_command(cmd);
	    
	    cli();
	    map->status.busy = 0;
	    sei();
	    have_command = 0;
	}
    }
    return 0;
}

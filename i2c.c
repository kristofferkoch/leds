#include "i2c.h"
#include "coroutine.h"
//#include CONFIG_H
#include <avr/io.h>

static void i2c_init(void) {
    TWSR = 0;
    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (0 << TWIE);
    TWAR = 72;//I2C_SLAVE_ADDR;
    TWAMR = 0;
}


void i2c_process(void) {
    CR_BEGIN();
    i2c_init();
    for (;;) {
	CR_WAIT(TWCR & (1 << TWINT));
	uint8_t twsr = TWSR & ~3;
	// NOTE: can't use switch here because of coroutine
	if (twsr == 0x60 || // Own SLA+W has been received; ACK has been returned
	    twsr == 0x68 || // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
	    twsr == 0x70 || // General call address has been received; ACK has been returned
	    twsr == 0x78) { // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
	    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Data byte will be received and ACK will be returned
	}
	else if (twsr == 0x80 || // Previously addressed with own SLA+W; data has been received; ACK has been returned
		 twsr == 0x90) { // Previously addressed with general call; data has been received; ACK has been returned
	    if (i2c_cb_receive(TWDR)) {
		TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Data byte will be received and ACK will be returned
	    }
	    else {
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWEN); // Data byte will be received and NOT ACK will be returned
	    }
	}
	else if (twsr == 0x88 || // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
		 twsr == 0x98) { // Previously addressed with general call; data has been received; NOT ACK has been returned
	    uint8_t data = TWDR;
	    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Switched to the not addressed Slave mode; own SLA will be recognized;
	    i2c_cb_receive(data);
	}		 
	else if (twsr == 0xa0) { // A STOP condition or repeated START condition has been received while still addressed as Slave
	    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Switched to the not addressed Slave mode; own SLA will be recognized;
	    i2c_cb_stop();
	}
	// Slave transmitter mode:
	else if (twsr == 0xa8 || // Own SLA+R has been received; ACK has been returned
		 twsr == 0xb0 || // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
		 twsr == 0xb8) { // Data byte in TWDR has been transmitted; ACK has been received
	    uint8_t data;
	    uint8_t ack = i2c_cb_transmit(&data);
	    TWDR = data;
	    if (ack) {
		TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Data byte will be transmitted and ACK should be received
	    }
	    else {
		TWCR = (1 << TWINT) | (0 << TWEA) | (1 << TWEN); // Last data byte will be transmitted and NOT ACK should be received
	    }
	}
	else if (twsr == 0xc0 || // Data byte in TWDR has been transmitted; NOT ACK has been received 
		 twsr == 0xc8) { // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received
	    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
	    i2c_cb_transmit_done(twsr == 0xc8);
	}
	// Misc mode:
	else if (twsr == 0x00) { // Bus error due to an illegal START or STOP condition
	    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWSTO);
	}
    }
    CR_END();
}

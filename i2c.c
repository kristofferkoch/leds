#include "i2c.h"
#include "coroutine.h"
//#include CONFIG_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

uint16_t i2c_count;

ISR(TWI_vect) {
    const uint8_t COMMON_TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
    uint8_t twsr = TW_STATUS;
    switch (twsr) {
#if 0
    case TW_START: // 0x08
    case TW_REP_START: // 0x10
    case TW_MT_SLA_ACK: // 0x18
    case TW_MT_SLA_NACK: // 0x20
    case TW_MT_DATA_ACK: // 0x28
    case TW_MT_DATA_NACK: // 0x30
    case TW_MT_ARB_LOST: // 0x38
    case TW_MR_ARB_LOST: // 0x38
    case TW_MR_SLA_ACK: // 0x40
    case TW_MR_SLA_NACK: // 0x48
    case TW_MR_DATA_ACK: // 0x50
    case TW_MR_DATA_NACK: // 0x58
    case TW_ST_ARB_LOST_SLA_ACK: // 0xB0
    case TW_SR_ARB_LOST_SLA_ACK: // 0x68
    case TW_SR_GCALL_ACK: // 0x70
    case TW_SR_ARB_LOST_GCALL_ACK: // 0x78
    case TW_SR_GCALL_DATA_ACK: // 0x90
    case TW_SR_GCALL_DATA_NACK: // 0x98
#endif
    case TW_ST_SLA_ACK: // 0xA8
	//i2c_count = 0;
    case TW_ST_DATA_ACK: { // 0xB8
	i2c_cb_want_transmit();
	return;
    }
    case TW_ST_DATA_NACK: // 0xC0
    case TW_ST_LAST_DATA: // 0xC8
	i2c_cb_transmit_done();
	return;
    case TW_SR_SLA_ACK: // 0x60
	//i2c_count = 0;
    case TW_SR_DATA_ACK: // 0x80
    case TW_SR_DATA_NACK: { // 0x88
	uint8_t ack = i2c_cb_receive(TWDR);
	if (twsr == TW_SR_DATA_ACK && !ack) {
	    TWCR = COMMON_TWCR;
	}
	else {
	    TWCR = COMMON_TWCR | (1 << TWEA);
	}
	return;
    }
    case TW_SR_STOP: // 0xA0
	TWCR = COMMON_TWCR | (1 << TWEA);
	i2c_cb_receive_done();
	return;
    case TW_BUS_ERROR: // 0x00
	TWCR = COMMON_TWCR | (1 << TWSTO);
    case TW_NO_INFO: // 0xF8
	return;
    default:
	for(;;); // Error. Hang until WDT resets
    }
}

void i2c_init(void) {
    TWCR = 0;
    TWSR = 0;
    i2c_count = 0;
    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
    TWAR = 72;//I2C_SLAVE_ADDR;
    TWAMR = 0;
}

void i2c_transmit(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
}

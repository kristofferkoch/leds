#include "i2c.h"
#include <stdint.h>

typedef struct {
    enum {
	I2C_IDLE,
	I2C_COMMAND_RECEIVED
    } state;
    uint8_t command;
} i2c_state_t;

i2c_state_t i2c_state;

uint8_t i2c_cb_receive(uint8_t data) {
    switch (i2c_state.state) {
    case I2C_IDLE:
	i2c_state.command = data;
	switch (data) {
	case CMD_ID:
	    i2c_state.state = I2C_COMMAND_RECEIVED;
	    return 1; // ACK
	default:
	    return 0; // NACK, invalid command
	}
    default:
	return 0; // NACK
    }
}

void i2c_cb_stop(void) {
    i2c_state.state = IDLE;
}

uint8_t i2c_cb_transmit(uint8_t *data) {
    switch (i2c_state.state) {
    default:
	*data = 0xff;
	return 0;
    }
}

void i2c_cb_transmit_done(uint8_t ack) {
}

int main(void) {
    for (;;) {
	i2c_process();
	// TODO: sleep
    }
    return 0;
}

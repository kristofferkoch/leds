#ifndef __I2C_H
#define __I2C_H

#include <stdint.h>
void i2c_process(void); // To be called in main loop

// Callbacks called from i2c_process:
uint8_t i2c_cb_receive(uint8_t data); // Called when bytes are received. Should return whether ack should be given
void i2c_cb_stop(void);
uint8_t i2c_cb_transmit(uint8_t *data);
void i2c_cb_transmit_done(uint8_t ack);

#endif

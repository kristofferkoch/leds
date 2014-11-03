#ifndef __I2C_H
#define __I2C_H

#include <stdint.h>

void i2c_init(void); // To be called in main loop

void i2c_transmit(uint8_t data);

// Callbacks called from i2c_process:
void i2c_cb_want_transmit(void);
void i2c_cb_transmit_done(void);
uint8_t i2c_cb_receive(uint8_t data);
void i2c_cb_receive_done(void);
#endif

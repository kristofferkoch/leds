#include "i2c.h"
#include "coroutine.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t rxdata_rdy;
volatile uint8_t rxdata;
volatile uint8_t txdata;

i2c_data_t i2c_cb_transmit(uint16_t count) {
    i2c_data_t ret;

    return ret;
}
void i2c_cb_transmit_done(void) {

}
uint8_t i2c_cb_receive(i2c_data_t data, uint16_t count) {
    rxdata = data.byte;
    rxdata_rdy = 1;
    return 1;
}
void i2c_cb_receive_done(void) {

}

static void process_command() {
    CR_BEGIN();
    for (;;) {
	CR_WAIT_ATOMIC(rxdata_rdy);
	uint8_t data = rxdata;
	sei();
    }
    CR_END();
}

int main(void) {
    i2c_init();
    rxdata_rdy = 0;
    sei();
    for (;;) {
	process_command();
	// TODO: sleep
    }
    return 0;
}

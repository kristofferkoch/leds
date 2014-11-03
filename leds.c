#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "color.h"

static const uint32_t FCPU = 16000000;
static const uint32_t BAUD = 800000;
#define UBRR (FCPU/(2*BAUD) - 1)
#define NLEDS 60

const uint8_t PROGMEM gamma[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
    115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
    144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
    177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
    215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

static void init_usart() {
    
    DDRD = (1<<PD4); // XCK pin
    
    UBRR0H = 0;
    UBRR0L = 0;

    UCSR0C = (1<<UMSEL01) | (1<<UMSEL00) | (1 << UCPHA0) | (0 << UCPOL0);
    UCSR0B = (1<<TXEN0);

    // Important: The baud rate must be set after the transmitter is enabled
    UBRR0H = UBRR>>8;
    UBRR0L = UBRR & 0xff;
}

static void start_timer8(uint8_t delay_8cycles) {
    TCCR0A = 0;
    TCNT0 = 0;
    TCCR0B = (2 << CS00); // div8 prescaler
    OCR0A = delay_8cycles;
    TIMSK0 = (1<<OCIE0A);
}

rgb_t buffer[2][NLEDS];
volatile uint8_t tx_current_buffer = 0;
volatile uint16_t tx_current_color;
volatile uint8_t tx_busy = 0;

static void update_transmit() {
    uint8_t cur = tx_current_buffer;
    uint8_t *buf = (uint8_t *)buffer[cur];
    
    uint16_t col = tx_current_color;
    if (col >= NLEDS*3) {
	// Transmission almost done, just wait 50us
	UCSR0B &= ~(1<<UDRIE0);
	tx_current_buffer = (cur + 1) % 2;
	
	UDR0 = 0;
	start_timer8(1000/8);
	return;
    }
    
    UCSR0B |= (1<<UDRIE0);
    UDR0 = buf[col];
    tx_current_color = col + 1;
}

static void start_transmit() {
    while(tx_busy) /*sleep*/;
    tx_busy = 1;
    tx_current_color = 2; // Hack for reshaping
    update_transmit();
}

ISR(USART_UDRE_vect)
{
    update_transmit();
}
ISR(TIMER0_COMPA_vect)
{
    tx_busy = 0;
    TCCR0B = 0; // Stop timer
    TIMSK0 = 0; // Disable interrupts
}




#define NEXT() do { state = __LINE__; return; case __LINE__:; } while (0)
#define BEGIN() static uint16_t state = 0;  switch (state) { case 0:;
#define END() } return

static void stars()
{
    int i;
    rgb_t *buf = buffer[(tx_current_buffer + 1) % 2];

    static uint16_t intensities[NLEDS];
    BEGIN();
    for (;;) {
      // Fade leds
      for (i = 0; i < NLEDS; i++) {
	if (intensities[i] > 0)
	  intensities[i] -= ((uint32_t)intensities[i])/1024;
      }
      if (rand() < RAND_MAX/300) {
	uint16_t led = rand() % NLEDS;
	uint16_t intens = 0x8000 + (rand() % 0x8000);
	if (intens > intensities[led])
	  intensities[led] = intens;
      }
      for (i = 0; i < NLEDS; i++) {
	uint8_t x = pgm_read_byte(&gamma[intensities[i]>>8]);
	buf[i].red = x;
	buf[i].green = x;
	buf[i].blue = x;
      }
      NEXT();
    }
    END();
}
static void rotating() {
    BEGIN();
    for (;;) {
      static uint8_t color;
      for (color = 1; color < 8; color++) {
	static uint8_t round;
	for (round = 0; round < 5; round++) {
	  static int16_t time;
	  for (time = -10; time < NLEDS; time++) {
	    static uint8_t delay;
	    for (delay = 0; delay < 5; delay++) {	      
	      uint16_t i;
	      rgb_t *buf = buffer[(tx_current_buffer + 1) % 2];
	      for (i = 0; i < NLEDS; i++) {
		// Leds turned on from time..time+10
		if (i-time < 10) {
		  buf[i].red = color & 1 ? 0xff:0;
		  buf[i].green = color & 2 ? 0xff:0;
		  buf[i].blue = color & 4 ? 0xff:0;
		}
		else {
		  buf[i].red = 0x1;
		  buf[i].green = 0x1;
		  buf[i].blue = 0x1;
		}
	      }
	      NEXT();
	    }
	  }
	}
      }
    }
    END();
}
static void hsv_test() {
    BEGIN();
    for (;;) {
      static uint16_t s;
      for (s = 2048; s != 0; s-=1) {
	static uint16_t v;
	for (v = 1024; v != 1023; v -= 1) {
	  uint16_t i;
	  rgb_t *buf = buffer[(tx_current_buffer + 1) % 2];
	      
	  for (i = 0; i < NLEDS; i += 1) {
	    uint16_t h = (i * 2048)/NLEDS;
	    const hsv_t hsv = {.h=h, .s=s, .v=v};
	    buf[i] = hsv2rgb(hsv);
	  }
	  NEXT();
	}
      }
    }
    END();
}

int main (void)
{
    init_usart();
    sei();
    
    for (;;) {
      stars();
      //rotating();
      //hsv_test();
      start_transmit();
    }
    
    return 0;
}

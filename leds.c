#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdlib.h>

static const uint32_t FCPU = 16000000;
static const uint32_t BAUD = 800000;
#define UBRR (FCPU/(2*BAUD) - 1)
#define NLEDS 60

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
    TCCR0B = (5 << CS00); // div8 prescaler
    OCR0A = delay_8cycles;
    TIMSK0 = (1<<OCIE0A);
}

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} color_t;

color_t buffer[2][NLEDS];
volatile uint8_t tx_current_buffer = 0;
volatile uint16_t tx_current_color = 0;
volatile uint8_t tx_busy = 0;

static void update_colors();

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
    tx_current_color = 0;
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


static void update_colors()
{
    int i;
    color_t *old = buffer[tx_current_buffer % 2];
    color_t *buf = buffer[(tx_current_buffer + 1) % 2];
#define NEXT() do { state = __LINE__; return; case __LINE__:; } while (0)
#define BEGIN() static uint16_t state = 0;  switch (state) { case 0:;
#define END() } return
    BEGIN();
    for (;;) {
	// Fade old leds
	for (i = 0; i < NLEDS; i++) {
	    uint16_t x = old[i].green;
	    x = ((x<<8) - (50*x))>>8;
	    buf[i].red = x;
	    buf[i].green = x;
	    buf[i].blue = x;
	}
	// Maybe light new led
	if (rand() < RAND_MAX/500) {
	    uint16_t led = rand() % NLEDS;
	    uint8_t intensity = rand();
	    if (old[led].green < intensity) {
		buf[led].red = intensity;
		buf[led].green = intensity;
		buf[led].blue = intensity;
	    }
	}
	NEXT();
    }
    END();
}
int main (void)
{
    init_usart();
    sei();
    
    for (;;) {
	update_colors();
	start_transmit();
    }
    
    return 0;
}

#ifndef _COROUTINE_H
#define _COROUTINE_H
#include <stdint.h>

#define CR_BEGIN()				\
    static uint8_t cr_state = 255;		\
    switch (cr_state) {				\
        case 255:

#define _CR_RETURN(cnt)				\
    do {					\
        cr_state = cnt;				\
	return;					\
        case cnt:;				\
    }						\
    while (0)

#define CR_RETURN() _CR_RETURN(__COUNTER__)

#define _CR_WAIT(cond, cnt)	       		\
	do {					\
	    cr_state = cnt;			\
	    case cnt:				\
	    if (!(cond))			\
		return;				\
	} while(0)
#define CR_WAIT(cond) _CR_WAIT(cond, __COUNTER__)

#define _CR_WAIT_ATOMIC(cond, cnt)      	\
	do {					\
	    cr_state = cnt;			\
	    case cnt:				\
	    cli();				\
	    if (!(cond)) {			\
	        sei();				\
		return;				\
	    }					\
	} while(0)
#define CR_WAIT_ATOMIC(cond) _CR_WAIT_ATOMIC(cond, __COUNTER__)

#define CR_END() }
#endif

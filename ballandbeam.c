/** stdio.h
 * AVR program for control of the DC-servo process.
 *  
 * User communication via the serial line. Commands:
 * s: start controller
 * t: stop controller
 * r: change sign of reference (+/- 5.0 volt)
 * 
 * * To compile for the ATmega8 AVR:
 *   avr-gcc -mmcu=atmega8 -O -g -Wall -o DCservo.o DCservo.c   
 * 
 * * To compile for the ATmega16 AVR:
 *   avr-gcc -mmcu=atmega16 -O -g -Wall -o DCservo.o DCservo.c   
 * 
 * To upload to the AVR:
 * avr-objcopy -Osrec DCservo.o DCservo.sr
 * uisp -dprog=stk200 --erase --upload if=DCservo.sr
 * 
 * To view the assembler code:
 * avr-objdump -S DCservo.o
 * 
 * To open a serial terminal on the PC:
 * simcom -38400 /dev/ttyS0 
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>


#define a 11		//Number of fractional bits


/* Controller parameters and variables (add your own code here) */

int8_t on = 0;                     /* 0=off, 1=on */
int16_t r = 1;                   /* Reference, corresponds to +10.0 V */
int16_t input;
int16_t temp;
int16_t k = 1;
int16_t y;

/** 
 * Write a character on the serial connection
 */
static inline void put_char(char ch){
  while ((UCSRA & 0x20) == 0) {};
  UDR = ch;
}

static inline void put_int(char channel, int16_t val)
{
	put_char(channel);
	put_char(':');
    
    int i;
    for (i = 1000; i > 0; i /= 10) {
        put_char('0' + val / i);
        val = val % i;
    }
	if( channel == 'p' || channel == 'i' ){ 
		put_char('\n');
	} else {
		put_char(' ');
	}
}


/**
 * Write 10-bit output using the PWM generator
 */
static inline void write_output(int16_t val) {
	//put_int('i',val);
  OCR1AH = (uint8_t) (val>>8);
  OCR1AL = (uint8_t) val;
}

/**
 * Read 10-bit input using the AD converter 
 */
static inline int16_t read_input(char chan) {
  uint8_t low, high;
  ADMUX = 0xc0 + chan;            /* Specify channel (0 or 1) */
  ADCSRA |= 0x40;                 /* Start the conversion */
  while (ADCSRA & 0x40);          /* Wait for conversion to finish */
  low = ADCL;                     /* Read input, low byte first! */
  high = ADCH;                    /* Read input, high byte */
  return (((high<<8) | low));	 /* 5 bit ADC value [-512..511] */ 
}  

/**c atoi
 * Interrupt handler for receiving characters over serial connection
 */
ISR(USART_RXC_vect){
    const char data = UDR;
    static uint16_t tem1, temp2, temp3, temp4,
    switch (data) {
        case 's':                        /* Start the controller */
        write_output(540);		//Sender 0 V
        on = 1;
    break;
    case 't':                        /* Stop the controller */
        write_output(540);		//Sender 0 V - optimized for exact control
        k = 1000;
        input = '\0';
        on = 0;
    break;
    default:
        if (k =1){
	        temp1 = (atoi(&data))*1000;
          k++;
        } else if (k = 2){
          temp2 = (atoi(&data))*100;
          k++;
        } else if(k = 3){
          temp3 = (atoi(&data))*10;
          k++;
        } else{
          temp 4 = (atoi(&data));
          k=0;
        }
	      write_output(temp1 + temp2 + temp3 + temp4);
        }
    break;
    }
}

/**
 * Interrupt handler for the periodic timer. Interrupts are generated
 * every 10 ms.
 */
ISR(TIMER2_COMP_vect){
	static int8_t channel = 0;
	if(on){
		channel = (channel == 0) ? 1 : 0;
		y = read_input(channel);
		put_int((channel == 0) ? 'p' : 'a',y);
	}
}

/**
 * Main program
 */
int main(){

  DDRB = 0x02;    /* Enable PWM output for ATmega8 */
  DDRD = 0x20;    /* Enable PWM output for ATmega16 */
  DDRC = 0x30;    /* Enable time measurement pins */
  ADCSRA = 0xc7;  /* ADC enable */

  TCCR1A = 0xf3;  /* Timer 1: OC1A & OC1B 10 bit fast PWM */
  TCCR1B = 0x09;  /* Clock / 1 */

  TCNT2 = 0x00;   /* Timer 2: Reset counter (periodic timer) */
  TCCR2 = 0x0f;   /* Clock / 1024, clear after compare match (CTC) */
  OCR2 = 144;     /* Set the compare value, corresponds to ~100 Hz */

  /* Configure serial communication */
  UCSRA = 0x00;   /* USART: */
  UCSRB = 0x98;   /* USART: RXC enable, Receiver enable, Transmitter enable */
  UCSRC = 0x86;   /* USART: 8bit, no parity */
  UBRRH = 0x00;   /* USART: 38400 @ 14.7456MHz */
  UBRRL = 23;     /* USART: 38400 @ 14.7456MHz */

  TIMSK = 1<<OCIE2; /* Start periodic timer */
  sei();          /* Enable interrupts */

  while (1);
}

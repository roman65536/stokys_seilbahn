/* AVR Node 84
This is the Node for my simple Home automatization network.

R.Pollak

The softuart code is take from Martin Thomas

*/

#define XSTR(x) STR(x)
#define STR(x) #x

#define WITH_STDIO_DEMO   1 /* 1: enable, 0: disable */



#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/wdt.h> 



#include <string.h>
#include <stdio.h>
#include "softuart.h"





void uart_putc(unsigned char ch);
unsigned char uart_getc(void );
void uart_init(long baud);
unsigned char  uart_kbhit(void);





char line[32];
char dbg[32];



long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void write(int fd, unsigned char *ptr,int len)
{
  DDRA |= 0x2;
  while(len--){
	uart_putc(*ptr++);
  }
  DDRA &= ~0x2;
}

void send2servo(int fd, unsigned char *data, int len)
{
  unsigned char ldata[len+3];
  unsigned char check_s=0;
  memcpy(&ldata[2],data,len);

  for(int a=0;a<len;a++) check_s+=data[a];
  ldata[len+2]=~check_s;
  ldata[0]=0xff;
  ldata[1]=0xff;
  write(fd,ldata,len+3);

}

void set2pos(int fd,char id ,int pos,int speed)
{
  unsigned char data[11];

 data[0]=id; //id
data[1]=0x09;
data[2]=0x03;
 data[3]=0x2a;
 data[4]= (pos>>8)&0xff;
 data[5]= pos&0xff;
 data[6]=0x0;
 data[7]=1;
 data[8]=(speed>>8)&0xff;
 data[9]=speed&0xff;
 send2servo( fd, data, 10);
 _delay_ms(80);
}



void adc_init(void )
{
//outp(0,SFIOR);
DIDR0=0x01;
ADMUXA=2;
ADCSRA=_BV(ADEN)| 7 ;
}


unsigned int adc_get(unsigned char ch )
{
 unsigned int ret;

 ADMUXA=ch;
 ADCSRA=_BV(ADEN)| _BV(ADSC) | 7;


 while(bit_is_set(ADCSRA,ADSC));

 ret= ADCW;
 return ret;
}



// interface between avr-libc stdio and software-UART
static int my_stdio_putchar( char c, FILE *stream )
{
	if ( c == '\n' ) {
	 uart_putc( '\r' );
	}
	 uart_putc( c );

	return 0;
}

FILE suart_stream = FDEV_SETUP_STREAM( my_stdio_putchar, NULL, _FDEV_SETUP_WRITE );

static void stdio_demo_func( void ) 
{
	stdout = &suart_stream;

		//printf_P( PSTR("This output done with printf_P\n") );
}

volatile unsigned int cnt1_ext;
volatile unsigned int baud;
volatile unsigned int sec;
inline unsigned long max(unsigned long a, unsigned long b) { return((a) > (b) ? a : b); }
inline unsigned long min(unsigned long a, unsigned long b) { return((a) < (b) ? a : b); }

ISR(TIMER1_COMPA_vect)
{
  cnt1_ext++;
  baud++;
  if(cnt1_ext > 999) {
		cnt1_ext=0;
		sec++;
		}
  soft_uart();
}

volatile unsigned int  a_intcnt;
volatile unsigned int end_b;

ISR(ANA_COMP1_vect)
{
  a_intcnt++;
}

int main(void)
{

  unsigned char ch;

  CCP=0xD8;
  CLKPR=0;

	
  //PORTA |= 0x2;
  PUEA = 0x2;
  DDRA = 0x40;
  // uart_init(9600);
  softuart_init(  );
  uart_init(1000000);
	
			
  //	adc_init();
   ACSR1A=(1<<ACIE1);
  ACSR1B= (1<<ACOE1);
  
  DIDR0=0x18;


  stdio_demo_func();

  TCCR1B		= 0x0b;
  OCR1A =	 125*2;
  TIMSK1		= 0x02;	
  cnt1_ext=0;
  sec=0;
  sei();
  softuart_turn_rx_on(  );
  wdt_enable (WDTO_500MS);
  printf_P(PSTR("RESET"));
//uart_putc('a');	
	for (;;) {
	
	  //	  	  volatile unsigned char flag=ACSR1A&0x20; 
	  //		  printf_P(PSTR("T%01x %d\n"),flag==0?1:0,sec );

	  if(softuart_kbhit()){
		ch=softuart_getchar();

		/* This is tricky 
		   there is one byte for command,
		   higher 4 bit is the Device ID 
		   lower part, 2 bit are for servos 1,2,3,4 reserved
		   bit 0 is to move particular servo to destination
		   bit 1 to move particular servo to 0 position
		*/
		if((ch & 0xf0) == (DEV_ID<<4) ) {
		  unsigned char servo = (ch & 0x0c) >> 2;
		  if((ch & 1) == 1) {
#pragma message "ID is " XSTR(DEV_ID)
#if DEV_ID < 3
			set2pos(0,servo+1,600,100);
#else
			set2pos(0,servo+1,600,300);
#endif
		  }
		  if((ch & 2) == 2) {
#if DEV_ID < 3
			set2pos(0,servo+1,0,300);
#else
			set2pos(0,servo+1,0,300);
#endif
		  }
			 
		}
		//			printf_P(PSTR("[%02x]"),softuart_getchar());		
	  }
	  wdt_reset();
		  // _delay_ms(10);
	  /*      		 
	  set2pos(0,1,600,100);
		  _delay_ms(8000);
		  set2pos(0,1,0,100);
		  _delay_ms(8000);
		  set2pos(0,2,600,100);
		  _delay_ms(8000);
		  set2pos(0,2,0,100);
		  _delay_ms(8000);
		  set2pos(0,2,600,100);
		  
		  set2pos(0,1,600,100);
		  _delay_ms(8000);
	  */
   	}
		
		

	
	return 0; /* never reached */
}


#if defined (HAVE_841)



void uart_init(long baud)
{   
unsigned char real=0;
  switch(baud)
  {
   case 2400: 
                real=108;
                break;

   case 9600:

                real=51;        /* 4mhz */
                break;
   case 19200:
                real=103;
                break;
   case 38400:
                real=12;
                break;

  case 1000000:
	real=0;
	
   }
#if ( __IO2333 | __IO4433 | _AVR_IO4433_H_)
//outp(real,UBRR);
//outp((1<<TXEN)|(1<<RXEN),UCSRB);
UBRR0=real;
UCSRB0=(1<<TXEN)|(1<<RXEN);
#else
//outp(real,UBRR);
//outp((1<<TXEN)|(1<<RXEN),UCR);
UBRR0L=real;
UCSR0C|=(1<<USBS0);
UCSR0B=(1<<TXEN0)|(1<<RXEN0);
// UCSR0A|=(1<<U2X0);
#endif
}


void uart_putc(unsigned char ch)
{

UCSR0A |= (1<<TXC0); 

//PORTA |= 0x40;
#if  (__IO2333 | __IO4433 | _AVR_IO4433_H_)
 while(bit_is_clear(UCSRA,UDRE)==1);
#else
 while(bit_is_clear(UCSR0A,UDRE0)==1);
#endif
// outp(ch,UDR);
UDR0=(ch );

// while(bit_is_clear(UCSR0A,TXC0)==1);
while (!(UCSR0A & (1 << TXC0)));
    
//_delay_us(1000);
//PORTA &= ~(0x40);
}


unsigned char uart_getc(void )
{
 
 unsigned char ch;
 while(bit_is_clear(UCSR0A,RXC0)==1);
 ch=UDR0;
 return(ch);
}

unsigned char  uart_kbhit(void)
{
if (bit_is_clear(UCSR0A,RXC0)==1) return 0;
 else return 1;
}


void prints( unsigned char * ch)
{
 unsigned char s;
 while((s=*ch++) !=  0) 
  uart_putc(s);
}


#endif

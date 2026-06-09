#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "softuart.h"

uint16_t EEMEM NonVolatileMINSPEED  = 55;
uint16_t EEMEM NonVolatileMAXSPEED = 255;
uint16_t EEMEM NonVolatileSEND_CMD_CNT = 2;
uint16_t EEMEM NonVolatileTOWER_STOP = 1465;
uint16_t EEMEM NonVolatileFAHR_CNT = 4 ;
uint16_t EEMEM NonVolatilePRESS_CNT = 0;
uint16_t EEMEM NonVolatileRUN_CNT=0;



#ifndef F_CPU
#define F_CPU 8000000UL
#endif


#ifndef BAUD
#define BAUD 38400
#endif
#include <util/setbaud.h>


//EEPROM Variables

int MAX_SPEED;
int  MIN_SPEED;
int SEND_CMD_CNT;
int TOWER_STOP;
int FAHR_CNT;
int PRESS_CNT;
int RUN_CNT;

#define SET_SERVO(b,se,st) (((b&0xf)<<4 ) | ((se&0x3) <<2) | st)
#define SERVO_OPEN 2
#define SERVO_CLOSE 1



volatile int min_speed;
volatile int maststop;

void eeprom_read()
{

  MIN_SPEED  = eeprom_read_word(&NonVolatileMINSPEED);
MAX_SPEED  = eeprom_read_word(&NonVolatileMAXSPEED);
SEND_CMD_CNT = eeprom_read_word(&NonVolatileSEND_CMD_CNT);
TOWER_STOP = eeprom_read_word(&NonVolatileTOWER_STOP);
 FAHR_CNT = eeprom_read_word(&NonVolatileFAHR_CNT);
 PRESS_CNT = eeprom_read_word(&NonVolatilePRESS_CNT);
 RUN_CNT = eeprom_read_word(&NonVolatileRUN_CNT);
}

/* http://www.cs.mun.ca/~rod/Winter2007/4723/notes/serial/serial.html */

void uart_init(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    
#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */ 
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */    
}

int uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

int uart_getchar(FILE *stream) {
  int ch;
  loop_until_bit_is_set(UCSR0A, RXC0);
  ch=UDR0;
  if(ch == '\r') ch='\n';
  
  uart_putchar(ch,stream);
    return ch;
}

int uart_check_char() {
  return bit_is_set(UCSR0A, RXC0);
}

volatile unsigned int cnt1_ext;
volatile unsigned int sec;
inline unsigned long max(unsigned long a, unsigned long b) { return((a) > (b) ? a : b); }
inline unsigned long min(unsigned long a, unsigned long b) { return((a) < (b) ? a : b); }
volatile unsigned int cnt_motor; 

ISR(TIMER1_COMPA_vect)
{
  cnt1_ext++;
  if(cnt1_ext > 999) {
		cnt1_ext=0;
		sec++;
		}
soft_uart();
}


ISR(INT0_vect)
{

  cnt_motor++;
}
 


unsigned long read_cntl()
{

  unsigned long cnt;
  // cnt=TCNT1;
  cnt= cnt1_ext;
  cnt= (cnt << 16) | TCNT1;
  return cnt;
}


void init_pwm()
{


  DDRD|=0x68;

  TCCR0A=_BV(COM0A1)|_BV(COM0B1)|_BV(WGM00);
  TCCR0B= 0x03;
  OCR0A=0;
  OCR0B=0;

}


/* 
	set_mot set motor speed -255 to 255 
*/

void set_mot(int speed)
{

  if(speed >=0) {
    OCR0A=speed; 
    OCR0B=0;
  }else{
    OCR0A=0;
    OCR0B=(speed *-1);
  }

}




void adc_init()
{
//outp(0,SFIOR);
DIDR0=1;
ADMUX=2;
ADCSRA=_BV(ADEN);
}


unsigned int adc_get(unsigned char mux)
{
 unsigned int ret;
 ADMUX=_BV(REFS0)|mux;
 ADCSRA=_BV(ADEN)| _BV(ADSC);
 while(bit_is_set(ADCSRA,ADSC));
 ret= ADCW;
 return ret;
}

FILE uart_io = FDEV_SETUP_STREAM(uart_putchar,uart_getchar, _FDEV_SETUP_RW);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#define TOP_L 1
#define TOP_R 2

#define UP_D 1
#define DOWN_D -1 

void readvar(int argn, char *argc[]);

void setvar(int argn, char *argc[])
{

  eeprom_read();
  if(argn == 1) {
	printf_P(PSTR("%s variable value\nminspeed (0-255)\nmaxspeed (0.255)\ncmdcnt (0-32768)\nmaststop (0-32768)\nfahrtcnt (0-32768)"),argc[0]);
	return;
  }
  if(!strcmp(argc[1],"minspeed")) MIN_SPEED=atoi(argc[2]);
  if(!strcmp(argc[1],"maxspeed")) MAX_SPEED=atoi(argc[2]);
  if(!strcmp(argc[1],"maststop")) TOWER_STOP=atoi(argc[2]);
  if(!strcmp(argc[1],"cmdcnt")) SEND_CMD_CNT=atoi(argc[2]);
  if(!strcmp(argc[1],"fahrtcnt")) FAHR_CNT=atoi(argc[2]);
  if(!strcmp(argc[1],"default")) {
	MIN_SPEED=55;
	MAX_SPEED=255;
	SEND_CMD_CNT=3;
	TOWER_STOP=1467;
	FAHR_CNT=4;
  }
  eeprom_write_word (&NonVolatileMINSPEED,  MIN_SPEED);
  eeprom_write_word (&NonVolatileMAXSPEED,  MAX_SPEED);
  eeprom_write_word (&NonVolatileSEND_CMD_CNT, SEND_CMD_CNT);
  eeprom_write_word (&NonVolatileTOWER_STOP,  TOWER_STOP);
  eeprom_write_word (&NonVolatileFAHR_CNT,  FAHR_CNT);
  readvar(argn,argc);
}

void readvar(int argn, char *argc[])
{
  eeprom_read();
  printf_P(PSTR("SPEED MIN[minspeed] %d MAX[maxspeed] %d\n"),MIN_SPEED,MAX_SPEED);
  printf_P(PSTR("Sending Commands[cmdcnt] %d times\n"),SEND_CMD_CNT);
  printf_P(PSTR("Tower Stop[maststop] at %d position\n"),TOWER_STOP);
  printf_P(PSTR("Runs per button[fahrtcnt] %d\n"),FAHR_CNT);
  printf_P(PSTR("Total Button Press %d\n"),PRESS_CNT);
  printf_P(PSTR("Total runs %d\n"),RUN_CNT);
}

void drive(int argn, char*argc[])
{

  int a,b;
 
  if(argn == 2)
        {
          a=atoi(argc[1]);
          set_mot(a);
		  printf_P(PSTR("%s set motor speed to %d\n"),__FUNCTION__,a);
        }
 
}

void readport(int argn, char *argc[])
{

  while(uart_check_char() == 0 )
	{
	  printf_P(PSTR("Portc is %x\n"),PINC);

	}
  uart_getchar((FILE *)0);
}

void checkpos(int argn, char *argc[])
{
  cnt_motor=0;
  sec=0;
while(uart_check_char() == 0 )
	{
	  printf_P(PSTR("Position is %d Clock is %d \n"),cnt_motor,sec);
	}
 uart_getchar((FILE *)0);

}

void sendtoservo(int argn, char * argc[] )
{
  int a,b,c;
  
  if(argn != 4 ) {
	printf_P(PSTR("more argument needed\n%s ID Servoid open|close\n"),argc[0]);
	return;
  }
  a=atoi(argc[1]);
  b=atoi(argc[2]);
  if(!strcmp(argc[3],"open")) c=SERVO_OPEN;
  else
  if(!strcmp(argc[3],"close")) c=SERVO_CLOSE;
  else goto s_error;
  if((a<0) || (a>15)) goto s_error;
  if((b<0) || (b>3))  goto s_error;
  softuart_putchar(SET_SERVO(a,b,c));
   	 _delay_ms(100);
	 return;

 s_error:
	 printf_P(PSTR("Error %s ID SERVO open|close \n"),argc[0]);
	 return;

}

void drive_to(int argn, char *argc[])
{

  if(argn != 3 ) {
	printf_P(PSTR("more argument needed\n%s position speed\n"),argc[0]);
	return;
  }
  int a=atoi(argc[1]);
  int b=atoi(argc[2]);

  if((b<-255) || (b>255)) {
	printf_P(PSTR("Error speed of of range\n"));
	return;
  }

   cnt_motor=0;
   set_mot(b);
   do {
	 printf_P(PSTR("Pos %d\n"),cnt_motor);
   }
   while((cnt_motor < a) && (uart_check_char() == 0));
   set_mot(0);
   _delay_ms(500);
    printf_P(PSTR("Pos %d\n"),cnt_motor);
  
}

void drive_home();
void test_doors(int );

void home(int argn, char *argc[])
{
  drive_home();
}
 
void testd(int argn,char *argc[])
{
  int a=atoi(argc[1]);
  test_doors(a);
}

void help(int argn, char *argc[]);


typedef

struct comtab {
  void (*fcn)(int argn ,char *argc[]);
  char cmd[10];
} COMTAB;

static COMTAB comtab[] = {


  //  {&ts, "ts" },
  //  {&draw_menu,"menu"},
  {&drive,"drive"},
  {&setvar,"set"},
  {&readvar,"readvar"},
  {&readport,"readport"},
  {&checkpos,"checkpos"},
  {&sendtoservo,"s2servo"},
  {&drive_to,"drive2"},
  {&home,"home"},
  {&testd,"testdoor"},
  {&help, "help"},
  {&help, "?"},
  { (void *)0 ,""}
};

static char *strparse(char **next, char *src)
{
  char *p;
  while (*src == ' ') {
    if (!(*src)) {
      return (char *)0;
    }
    src++;
  }
  p = src;
  while ((*src != ' ') && *(src++));
  *(src++) = 0;
  *next = src;
  return p;
}

static int comidx(char *str)
{
  int idx = 0;

  while (comtab[idx].fcn) {
    if (!strcmp(comtab[idx].cmd,str)) return idx;

    idx++;
  }
  return -1;
}

void shell()
{
  char buf[80];
  char *args[6];
  char *next;
  char *bufend;
  int command;
  int i;

  printf_P(PSTR("Debug shell\nBe very carefull what you are doing!! Beat!! Lucas you too!!\n"));
	  for (;;) {
		printf_P(PSTR("\n>"));
		fgets(buf,80,stdin);
		int a=strlen(buf);
		if(a>0) buf[a-1]=0;
                              
		//printf_P(PSTR("%s  [%s]\n"),__FUNCTION__,buf);
		bufend = buf + strlen(buf);

		i = 0;
		next = buf;

		while (next < bufend) {
		  args[i] = strparse(&next, next);

		  if (!strcmp(";",args[i++]) || !(next < bufend)) {

			if (!strcmp("exit",args[0])) return;

			if (*args[0] != '\n' && *args[0] != '\r' && strlen(args[0])) {
			  if ((command = comidx(args[0])) == -1) {
				printf_P(PSTR("sh: %s command not found."),buf);
			  }
			  else {

				args[i] = (void *)0;

			
				comtab[command].fcn(i,(void *) args);

				i = 0;
			  }
			}
		  }
		}



	  }

}

  void help(int argn, char *argc[])
{
  int idx=0;
   while (comtab[idx].fcn) {
	 printf_P(PSTR("%s \n"),comtab[idx].cmd);
         idx++;
   }
}


/*
hier wird das Kommando gesendet zu den Gondeln und zum Tower
Es besteht aus einem Byte. SET_SERVO beschreibt, zur welcher ID (Gondel rechts, links, Tower ), welches servo (Innen oder Aussen), ob aufmachen oder zu machen.
*/

void send_cmd(unsigned char ch)
{
  int a;
  for(a=0;a<SEND_CMD_CNT;a++)
	{
	  softuart_putchar(ch);
	  _delay_ms(100);
	}
}


/*
Sequenz zum öffnen der Türen bei den Gondeln
Wichtig dabei, gesehen von Tallstation, Gondel rechts hat ID 1, Innen tür ID 0, aussen Tür ID 1
Gondel links hat ID 2, Innen Tür ID 0, aussen Tür ID 2
*/



void open_doors(int lr)
{
   if(lr == UP_D)
	{
  printf("%s \n",__FUNCTION__);
  send_cmd(SET_SERVO(1,1,SERVO_OPEN));
  _delay_ms(300);
  send_cmd(SET_SERVO(1,1,SERVO_OPEN));
  	_delay_ms(500);
	send_cmd(SET_SERVO(2,0,SERVO_OPEN));
  _delay_ms(8000);
   _delay_ms(300);
  send_cmd(SET_SERVO(1,0,SERVO_OPEN));
 _delay_ms(300);
  send_cmd(SET_SERVO(1,1,SERVO_CLOSE));
  _delay_ms(8000);
   _delay_ms(300);
  send_cmd(SET_SERVO(1,0,SERVO_CLOSE));
   _delay_ms(300);
  send_cmd(SET_SERVO(2,0,SERVO_CLOSE));
  _delay_ms(8000);
	} else {
	 send_cmd(SET_SERVO(2,1,SERVO_OPEN));
  _delay_ms(300);
  send_cmd(SET_SERVO(2,1,SERVO_OPEN));
  	_delay_ms(500);
	send_cmd(SET_SERVO(1,0,SERVO_OPEN));
  _delay_ms(8000);
   _delay_ms(300);
  send_cmd(SET_SERVO(2,0,SERVO_OPEN));
 _delay_ms(300);
  send_cmd(SET_SERVO(2,1,SERVO_CLOSE));
  _delay_ms(8000);
   _delay_ms(300);
  send_cmd(SET_SERVO(2,0,SERVO_CLOSE));
   _delay_ms(300);
  send_cmd(SET_SERVO(1,0,SERVO_CLOSE));
  _delay_ms(8000);
   }
}


void close_doors()
{
   printf("%s \n",__FUNCTION__);
  /*   send_cmd(SET_SERVO(1,1,SERVO_CLOSE)); */
  /* _delay_ms(300); */
  /* send_cmd(SET_SERVO(1,1,SERVO_CLOSE)); */
  /* _delay_ms(8000); */
}

void close_all_doors()
{
  send_cmd(SET_SERVO(1,1,SERVO_CLOSE));
  send_cmd(SET_SERVO(1,0,SERVO_CLOSE));
  send_cmd(SET_SERVO(2,1,SERVO_CLOSE));
  send_cmd(SET_SERVO(2,0,SERVO_CLOSE));

}

/* 
Sequenz wenn beim Tower angehalten wird. 
Welche Klappe nach unten geht und welche Tür aufgeht bei der Gondel

Wichtig bei Tower, da die Servos um 180 grad eingebaut sind, ist auf der rechten Seite geht mit SERVO_CLOSE 
die Klappe nach unten. Mit SERVO_OPEN geht die klappe wieder nach oben 
Auf linken Seite geht mit SERVO_OPEN die Klappe nach unten..

*/

void tower_seq(int lr)
{

  if(lr == UP_D)
	{
	  _delay_ms(500);
	  send_cmd(SET_SERVO(3,0,SERVO_CLOSE));
	  _delay_ms(500);
	  send_cmd(SET_SERVO(1,0,SERVO_OPEN));
	  _delay_ms(8000);
			  
	  send_cmd(SET_SERVO(1,0,SERVO_CLOSE));
	  _delay_ms(8000);
	  send_cmd(SET_SERVO(3,0,SERVO_OPEN));
	  _delay_ms(3000);


	} else {
	send_cmd(SET_SERVO(3,1,SERVO_OPEN));
	_delay_ms(500);
	send_cmd(SET_SERVO(2,0,SERVO_OPEN));
	_delay_ms(8000);
			  
	send_cmd(SET_SERVO(2,0,SERVO_CLOSE));
	_delay_ms(8000);
	send_cmd(SET_SERVO(3,1,SERVO_CLOSE));
	_delay_ms(3000);



  }
}

void drive_home()
{

   /* 
Definierte Ausgangslage für die Bahn.
Die Linke Gondel soll in der Tallstation sein
 */
 
 if((PINC & TOP_R) !=0 )
   {
	 set_mot(MIN_SPEED);
	 do {} while((PINC & TOP_R) != 0);
	 set_mot(0);
   }
 cnt_motor=0;

}

void test_doors(int nr_t)
{
  int a;
   for(a=0;a<nr_t;a++){
   send_cmd(SET_SERVO(1,1,SERVO_OPEN));
   _delay_ms(100);
   send_cmd(SET_SERVO(1,0,SERVO_OPEN));
   _delay_ms(100);
   send_cmd(SET_SERVO(2,1,SERVO_OPEN));
   _delay_ms(100);
   send_cmd(SET_SERVO(2,0,SERVO_OPEN));
   _delay_ms(100);
   //send_cmd(SET_SERVO(1,1,SERVO_OPEN));
   send_cmd(SET_SERVO(3,0,SERVO_CLOSE));
   _delay_ms(100);
   send_cmd(SET_SERVO(3,1,SERVO_OPEN));
   _delay_ms(8000);
   send_cmd(SET_SERVO(1,0,SERVO_CLOSE));
   _delay_ms(100);
   send_cmd(SET_SERVO(1,1,SERVO_CLOSE));
   _delay_ms(100);
   send_cmd(SET_SERVO(3,0,SERVO_OPEN));
   _delay_ms(100);
   send_cmd(SET_SERVO(3,1,SERVO_CLOSE));
   _delay_ms(100);
   send_cmd(SET_SERVO(2,0,SERVO_CLOSE));
   _delay_ms(100);
   send_cmd(SET_SERVO(2,1,SERVO_CLOSE));
   //send_cmd(0x16);
   //send_cmd(SET_SERVO(1,1,SERVO_OPEN));
   _delay_ms(8000);
 }
}


void main()
{
  int a;
   char drive=DOWN_D;
  //int speed=255; //max speed
  int speed=128;

 CLKPR=_BV(CLKPCE);
 CLKPR=0;

  
 _delay_ms(500);
 init_pwm();
 //adc_init();
 DDRD   |=  2;  // Relay and tx output
 DDRC =0;      // all input
 PORTC = 0xff;   // set pull up for C0,C1



	
 uart_init();
 stdout = stdin = &uart_io;
 softuart_init();
 EICRA=0x3; // T0 interrupt on falling 
 EIMSK=1;  // T0 interrupt enable
 cnt_motor=0;

 //TCCR1A		= 0x00;
 TCCR1B		= 0x0b;
 OCR1A =	 125;
 TIMSK1		= 0x02;	
 cnt1_ext=0;
 sec=0;


 printf_P(PSTR("\n\n\nPollak Engineering \nWelcome\n"));
  _delay_ms(500);
  eeprom_read();
 
sei();
 if((PINC & 0x08) == 0) shell();
  printf_P(PSTR("Testing Doors\n"));
 test_doors(2);
  printf_P(PSTR("Driving Home\n"));
 drive_home();

 
 // close_all_doors();

 /*
Loop bis in aller Ewigkeiten 
 */
  for(;;)
    {
	  printf_P(PSTR("Ready to Press Start\n"));
	  /*
Warte bis Start gedrückt wird
	  */
	  do { if((PINC & 0x08) == 0) shell(); } while((PINC & 0x4) != 0) ;
	  printf_P(PSTR("GO\n"));
	  set_mot(speed);
	  PRESS_CNT++;
	  eeprom_write_word (&NonVolatilePRESS_CNT,  PRESS_CNT);

	  /*
		Fahre (wie im eeprom konfiguriert) mal insgesammt 
	  */
	  
	  for(a=0;a<FAHR_CNT;) {
		unsigned char pinc=PINC;

		/* beim Tower längsämer fahren */
		if((cnt_motor >1200) && (cnt_motor <1700))
		  {
			if((speed > MIN_SPEED ) && (cnt_motor <1400)) {
			  if((cnt1_ext & 0x3) == 0)
				speed--;
			}
			if(cnt_motor == TOWER_STOP) {             // << BEI DER POSITION wird angehalten
			  set_mot(0);
			  tower_seq(drive);
			}
		  }
		else
	  		if(cnt_motor > 2100) {             // Wenn in der näche der Bergstation motoren verlangsamen
		  if(speed > MIN_SPEED ) {
			if((cnt1_ext & 0x3) == 0)
			  speed--;
		  }
		}
		else {
		if(cnt_motor <250) speed=MIN_SPEED; // wenn aus der Tallstation raus, beschleunigen
		else if (speed < MAX_SPEED) {
		  if((cnt1_ext & 0x3) == 0)
			speed++;
		 }
		}

		//	 printf("%d %d %d %x %d %d\n",speed*drive,cnt1_ext,speed,pinc,cnt_motor,a); 
	  printf("%d %d %d\n",speed,cnt1_ext,cnt_motor);

	  if(cnt_motor > 10) {
		 
		if(((pinc & TOP_R) == 0) && (drive == UP_D)) {               // testen, is Gondel Rechts auf Endschalter ??
		
		  set_mot(0);
		  drive=DOWN_D;
	
		  printf("*******************************\n%d\n\n",cnt_motor);
		  cnt_motor=0;
		  //	  if(a >0) {
			_delay_ms(1000);
			open_doors(drive);
			close_doors();
			//  }
		  a++;
		  RUN_CNT++;
		  eeprom_write_word (&NonVolatileRUN_CNT,  RUN_CNT);
		}
	  
		if(((pinc & TOP_L) == 0) && (drive == DOWN_D)) {          // testen, is Gondel Links auf Endschalter ??
	
		  set_mot(0);
		  drive=UP_D;
		
		  printf("*******************************\n%d\n\n",cnt_motor);
		  cnt_motor=0;
		  // if(a>0) {
			_delay_ms(1000);
			open_doors(drive);
			close_doors();
			// }
		  a++;
		   RUN_CNT++;
		  eeprom_write_word (&NonVolatileRUN_CNT,  RUN_CNT);
		}
	  }
	  set_mot(speed*drive);                           // setze die geschwindigkeit unter berücksichtigung der Richtung
	  
	 
	  }
	  
	  set_mot(0);  //Sequenz ende -> motoren anhalten
	}
}

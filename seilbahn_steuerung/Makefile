CC = avr-gcc
CFLAGS = -mmcu=atmega168 -O6 -DLOGGER -DPRINT -DF_CPU=8000000
#CFLAGS = -mmcu=atmega8 -Os -DLOGGER -DPRINT -DF_CPU=1000000
#CFLAGS = -mmcu=at90s2333 -O2 -DLOGGER -DPRINT
OBP = avr-objcopy
#OFLAGS = --remove-section .eeprom -O ihex
OFLAGS = -O ihex




.c.o :
	$(CC) $(CFLAGS) -c $< -o $@


SRC = seil.c softuart.c
OBJ = seil.o softuart.o
ELF = seil 


all:   $(OBJ)
	$(CC) $(CFLAGS) -o $(ELF) $(OBJ)
	$(OBP) -R .eeprom $(OFLAGS) $(ELF) seil.hex
	$(OBP) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O ihex $(ELF) seil.eep 



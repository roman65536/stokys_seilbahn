.DEFAULT_GOAL := all
CC = avr-gcc
CFLAGS = -mmcu=attiny841 -I. -gdwarf-2 -DF_CPU=16000000UL  -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wstrict-prototypes  -std=gnu99 -ffunction-sections -fdata-sections -fno-inline-small-functions -DHAVE_841


softuart.o: softuart.c
	$(CC) $(CFLAGS) -c $<

main_1.o:	main.c
	$(CC) $(CFLAGS) -DDEV_ID=1 -c main.c -o main_1.o

main_2.o:	main.c
	$(CC) $(CFLAGS) -DDEV_ID=2 -c main.c -o main_2.o

main_3.o:	main.c
	$(CC) $(CFLAGS) -DDEV_ID=3 -c main.c -o main_3.o

all:	softuart.o main_1.o main_2.o main_3.o
	$(CC) main_1.o softuart.o --output main_1.elf -Wl,-u,vfprintf -lprintf_min  -lm -Wl,--gc-sections -Wl,--relax
	$(CC) main_2.o softuart.o --output main_2.elf -Wl,-u,vfprintf -lprintf_min  -lm -Wl,--gc-sections -Wl,--relax
	$(CC) main_3.o softuart.o --output main_3.elf -Wl,-u,vfprintf -lprintf_min  -lm -Wl,--gc-sections -Wl,--relax
	avr-objcopy -O ihex  -R .fuse -R .lock -R .signature main_1.elf main_1.hex
	avr-objcopy -O ihex  -R .fuse -R .lock -R .signature main_2.elf main_2.hex
	avr-objcopy -O ihex  -R .fuse -R .lock -R .signature main_3.elf main_3.hex

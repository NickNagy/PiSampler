CC=gcc
CFLAGS=-g -I/opt/vc/include -L/opt/vc/lib -lbcm_host -o

all: main dma-test

main: main.c
	$(CC) $(CFLAGS) main bcm/gpio.c bcm/clk.c bcm/dma.c bcm/pcm.c bcm/pimem.c globals.c main.c

dma-test: dma-test.c
	$(CC) $(CFLAGS) dma-test bcm/dma.c bcm/pimem.c globals.c dma-test.c

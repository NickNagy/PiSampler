# GCC / CFLAGS guide:
#	-g 	  = GDB
#	-o    = specify name of output file
#	-Wall = enable all GCC warnings
#	-S	  = generate the assembly file
#	-C	  = generate .o file w/o any linking
#	-l	  = "-llibrary" --> search the library when linking
#	-L	  = "-Ldir" --> add directory to list of directories to be searched by -l
#	-I	  = "-Idir" --> add directory to list of directories to be searched for header files

CC=g++

VC_IDIR = /opt/vc/include
VC_LDIR = /opt/vc/lib

CFLAGS = -g #-c
IFLAGS = -I$(VC_IDIR)
LFLAGS = -L$(VC_LDIR) -lbcm_host 

DMA_TEST_DEPS = bcm/dma.h bcm/pimem.h globals.h 
DMA_TEST_OBJS = bcm/dma.o bcm/pimem.o 
DMA_TEST_CFILES = bcm/dma.c bcm/pimem.cpp 

MAIN_DEPS = $(DMA_TEST_DEPS) bcm/gpio.h bcm/clk.h bcm/pcm.h 
MAIN_OBJS = $(DMA_TEST_OBJS) bcm/gpio.o bcm/clk.o bcm/pcm.o
MAIN_CFILES = $(DMA_TEST_CFILES) bcm/gpio.c bcm/clk.c bcm/pcm.c

all: main dma-test

main: main.c $(MAIN_CFILES)
	$(CC) $^ $(IFLAGS) $(LFLAGS) $(CFLAGS) -o $@ 

dma-test: dma-test.c $(DMA_TEST_CFILES)
	$(CC) $^ $(IFLAGS) $(LFLAGS) $(CFLAGS) -o $@ 
	
clean:
	rm -rf dma-test main

.PHONY: clean

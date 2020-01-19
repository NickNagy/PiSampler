# GCC / CFLAGS guide:
#	-g 	  = GDB
#	-o    = specify name of output file
#	-Wall = enable all GCC warnings
#	-S	  = generate the assembly file
#	-C	  = generate .o file w/o any linking
#	-l	  = "-llibrary" --> search the library when linking
#	-L	  = "-Ldir" --> add directory to list of directories to be searched by -l
#	-I	  = "-Idir" --> add directory to list of directories to be searched for header files

CC=gcc

VC_IDIR = /opt/vc/include
VC_LDIR = /opt/vc/lib

CFLAGS = -g
IFLAGS = -I$(VC_IDIR)
LFLAGS = -L$(VC_LDIR) -lbcm_host 

DMA_TEST_DEPS = bcm/dma.h bcm/pimem.h globals.h 
DMA_TEST_OBJS = bcm/dma.o bcm/pimem.o globals.o

MAIN_DEPS = $(DMA_TEST_DEPS) bcm/gpio.h bcm/clk.h bcm/pcm.h 
MAIN_OBJS = $(DMA_TEST_OBJS) bcm/gpio.o bcm/clk.o bcm/pcm.o

all: main dma-test

main: main.o $(MAIN_OBJS)
	$(CC) $^ $(IFLAGS) $(LFLAGS) $(CFLAGS) -o $@ 

dma-test: dma-test.o $(DMA_TEST_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(IFLAGS) $(LFLAGS) 

clean:
	rm -rf dma-test main

.PHONY: clean

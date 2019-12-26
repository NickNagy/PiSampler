#include "pcm.h"

// TODO: check where the page actually starts..
static unsigned * pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE);

static bool pcmInitialized = 0;
static bool pcmRunning = 0;

static char pcmMode;
static int syncWait;

// TODO
static void checkFrameAndChannelWidth(char frameLength, char dataWidth) {
    return 0;
}

/*
determine the delay (in microseconds) between writing to SYNC in PCM CTRL reg
and reading back the value written. This corresponds to ~2 PCM clk cycles

should only need to call once to get an idea of how long to delay the program
*/ 
static int getSyncDelay() {
    char sync, notSync;
    timeval initTime, endTime;
    // get current sync value
    sync = (pcmMap(PCM_CTRL_REG) >> 24) & 1;
    notSync = sync^1;
    // write opposite value back to reg
    pcmMap(PCM_CTRL_REG) &= ~(sync<<24);
    // time
    gettimeofday(&initTime);
    while ((pcmMap(PCM_CTRL_REG) >> 24) & notSync);
    gettimeofday(&endTime);
    int usElapsed = ((endTime.tv_sec - initTime.tv_sec)*1000000) + (endTime.tv_usec - startTime.tv_usec);
    // round up
    return usElapsed + (10 - (usElapsed % 10));
}

static bool checkInitParams(char mode, bool clockMode, char numChannels, char frameLength, char dataWidth, unsigned char thresh) {
    bool error = 0;
    if (mode < 0 || mode > 2) {
        printf("ERROR: please select from the following for MODE:\n\t0: polling mode\n\t1: interrupt mode\n\t\2: DMA mode\n");
        error = 1;
    }
    if (!clockMode && !checkFrameAndChannelWidth(frameLength, dataWidth)) {
        printf("ERROR: incompatible frame lengths and data widths.\n");
        error = 1;
    }
    if (numChannels < 1 || numChannels > 2) {
        printf("ERROR: valid number of channels are 1 or 2.\n");
        error = 1;
    }
    if (dataWidth > 32) {
        printf("ERROR: maximum data width available is 32 bits.\n");
        error = 1;
    } else if (dataWidth % 8) {
        printf("ERROR: please set data width to be a multiple of 8 bits.\n");
        error = 1;
    }
    if (mode < 2 && thresh > 3) {
        printf("ERROR: threshold must be two-bit value for poll or interrupt mode.\n");
        error = 1;
    } 
    if (mode == 2 && thresh >= 128) {
        printf("ERROR: threshold must be six-bit value for DMA mode.\n");
        error = 1;
    }
    return !error;
}

static void initRXTXControlRegisters(bool clockMode, char numChannels, char dataWidth) {
    char widthBits, widthExtension;
    unsigned short channel1Bits, channel2Bits;   
    int txrxInitBits;
    /*
    if PMOD is in master mode then only SSM is supported, which means if we have 2 channels
    we want 16b data and packed mode set
    */
    if (clockMode && numChannels==2) {
        if (dataWidth > 16) {
            printf("\nTruncating channel width to 16 bits...\n");
            dataWidth = 16;
        }
        pcm(PCM_MODE_REG) |= 3 << 24; // set FRXP & FTXP if not already set
    }
    widthBits = (dataWidth > 24) ? dataWidth - 24: dataWidth - 8;
    widthExtension = (dataWidth > 24) ? 1: 0;
    channel1Bits = (widthExtension << 15) | (1 << 4) | widthBits; // start @ clk 1 of frame
    channel2Bits = channel1Bits | (32 << 4); // start 32 bits after ch1 (assuming SCLK/LRCLK == 64)
    txrxInitBits = (channel1Bits << 16) | channel2Bits;
    pcmMap(PCM_RXC_REG) = txrxInitBits;
    pcmMap(PCM_TXC_REG) = txrxInitBits;
}

/*
mode = 0 --> polled mode
mode = 1 --> interrupt mode
mode = 2 --> DMA mode

clockMode: 
    0 = the PCM clk is an output and drives at the MCLK rate
    1 = the PCM clk is an input
fallingEdgeInput: used for programming CLKI in MODE reg
    0 = data is clocked in on rising edge of clk and outputted on falling edge
    1 = data is clocked in on falling edge of clk and outputted on rising edge
frameLength: ** irrelevant when clockMode==1 **
dataWidth: width of data per channel. Should be max 32 bits, and should be a multiple of 8 bits
thresh:
    TX and RX threshold for when TXW and RXW flags should be set. Only relevant for poll or interrupt mode
    00 = set when FIFO is empty
    ...
    11 = set when FIFO is full
*/
void initPCM(char mode, bool clockMode, bool fallingEdgeInput, char numChannels, char frameLength, char dataWidth, unsigned char thresh) {
    if (pcmRunning) {
        printf("ERROR: PCM interface is currently running.\nAborting...\n");
        return;
    }
    if (!checkInitParams(mode, clockMode, numChannels, frameLength, dataWidth, thresh)) {
        printf("Aborting...\n");
        return;
    }
    printf("Initializing PCM interface...");
    pcmMap(PCM_CTRL_REG) |= 1; // enable set
    // CLKM == FSM
    pcmMap(PCM_MODE_REG) = ((clockMode << 23) | (fallingEdgeInput << 22) | (clockMode << 21) | (frameLength << 10) | frameLength);
    initRXTXControlRegisters(clockMode, numChannels, dataWidth);
    // assert RXCLR & TXCLR, wait 2 PCM clk cycles
    pcmMap(PCM_CTRL_REG) |= TXCLR | RXCLR;
    syncWait = getSyncDelay();
    switch(mode) {
        case 1: // interrupt
        {
            pcm(PCM_CTRL_REG) |= (thresh << 7) | (thresh << 5);
            pcm(PCM_INTEN_REG) |= 3; // enable interrupts
            break;
        }
        case 2: // DMA // TODO
        {
            pcm(PCM_CTRL_REG) |= (1 << 9); // DMAEN
            pcm(PCM_DREQ_REG) |= (thresh << 8) | thresh;
            // TODO: dma.h
            break;
        }
        default: // polling
        {
            pcm(PCM_CTRL_REG) |= (thresh << 7) | (thresh << 5); 
            break;
        }
    }
    pcmMode = mode;
    pcmInitialized = 1;
    printf("done.\n");
}

void directLineInLineOut() {
    switch(pmMode) {
        case 1: { // interrupt
            break;
        }
        case 2: { // DMA
            break;
        }
        default: { // polling
            while(1) {
                // wait for enough data in RX to be receivable
                while(!(pcmMap(PCM_INTSTC_REG) & 2));
                pcmMap(PCM_CTRL_REG) &= RXOFFTXON;
                while(!(pcmMap(PCM_INTSTC_REG) & 1));
                pcmMap(PCM_CTRL_REG) &= RXONTXOFF;
            }
            break;
        }
    }
}

/* 
Polling mode:
    - if transmitting, ensure sufficient sample words have been written to PCMFIFO before
        transmission starts.
    - poll TXW when writing sample words from FIFO and RXR when reading sample words until all
        data is transferred
Interrupt mode:
    - if transmitting, ensure sufficient sample words have been written to PCMFIFO before
        transmission starts.
    - when an interrupt occurs, check RXR. If this is set, one or more sample words are available in the FIFO.
        If TXW is set then one or more sample words can be sent to the FIFO
DMA mode:

*/
// TODO: start in seperate thread?
void startPCM(char runMode) {
    if (!pcmInitialized) {
        printf("ERROR: PCM interface has not been initialized yet.\n");
        return;
    }
    int data;
    pcmRunning = 1;
    // NOTE: transmit FIFO should be pre-loaded with data
    pcmMap(PCM_FIFO_REG) = 0;
    //pcmMap(PCM_CTRL_REG) |= 6; // set TXON and RXON
    pcmMap(PCM_CTRL_REG) &= RXONTXOFF;
    switch(pcmMode) {
        case 1: { // interrupt
            break;
        }
        case 2: { // DMA
            break;
        }
        default: { // polling
            while(1) {
                // wait for enough data in RX to be receivable
                while(pcmMap(PCM_INTSTC_REG) & 2);
                pcmMap(PCM_CTRL_REG) &= RXOFFTXOFF;
                data = pcmMap(PCM_FIFO_REG);
                // check for when enough space in TX to write data to
                pcmMap(PCM_FIFO_REG) = data;
                pcmMap(PCM_CTRL_REG) &= RXOFFTXON;
                pcmMap(PCM_CTRL_REG) &= RXOFFTXOFF;
            }
            break;
        }
    }
}
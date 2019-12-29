#include "pcm.h"

static unsigned * pcmMap; 
static bool pcmInitialized;
static bool pcmRunning;

static char pcmMode;
static int syncWait;

// TODO
static bool checkFrameAndChannelWidth(pcmExternInterface * ext) {
    return 0;
}

static bool checkInitParams(pcmExternInterface * ext, unsigned char thresh, char mode) {
    bool error = 0;
    if (mode < 0 || mode > 2) {
        printf("ERROR: please select from the following for MODE:\n\t0: polling mode\n\t1: interrupt mode\n\t\2: DMA mode\n");
        error = 1;
    }
    if (!ext->isMasterDevice && !checkFrameAndChannelWidth(ext)) {
        printf("ERROR: incompatible frame lengths and data widths.\n");
        error = 1;
    }
    if (ext->numChannels < 1 || ext->numChannels > 2) {
        printf("ERROR: valid number of channels are 1 or 2.\n");
        error = 1;
    }
    if (ext->dataWidth > 32) {
        printf("ERROR: maximum data width available is 32 bits.\n");
        error = 1;
    } else if (ext->dataWidth % 8) {
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

/*
determine the delay (in microseconds) between writing to SYNC in PCM CTRL reg
and reading back the value written. This corresponds to ~2 PCM clk cycles

should only need to call once to get an idea of how long to delay the program
*/ 
static int getSyncDelay() {
    char sync, notSync;
    struct timeval initTime, endTime;
    // get current sync value
    sync = (pcmMap[PCM_CTRL_REG] >> 24) & 1;
    notSync = sync^1;
    // write opposite value back to reg
    pcmMap[PCM_CTRL_REG] &= ~(sync<<24);
    // time
    gettimeofday(&initTime, NULL);
    while ((pcmMap[PCM_CTRL_REG] >> 24) & notSync);
    gettimeofday(&endTime, NULL);
    int usElapsed = ((endTime.tv_sec - initTime.tv_sec)*1000000) + (endTime.tv_usec - initTime.tv_usec);
    DEBUG_VAL("2-cycle PCM clk delay in micros", (usElapsed + (10 - (usElapsed % 10))));
    // round up
    return usElapsed + (10 - (usElapsed % 10));
}

static void initRXTXControlRegisters(pcmExternInterface * ext) {
    char widthBits, widthExtension, ch2Enable;
    unsigned short channel1Bits, channel2Bits;   
    int txrxInitBits;
    ch2Enable = 0;
    if (ext->numChannels==2) {
        ch2Enable = 1;
        /*
        if PMOD is in master mode then only SSM is supported, which means if we have 2 channels
        we want 16b data and packed mode set
        */
        if (ext->isMasterDevice) {
            if (ext->dataWidth > 16) {
                printf("\nTruncating channel width to 16 bits...\n");
                ext->dataWidth = 16;
            }
            pcmMap[PCM_MODE_REG] |= 3 << 24; // set FRXP & FTXP if not already set
        }    
    }
    widthBits = (ext->dataWidth > 24) ? ext->dataWidth - 24: ext->dataWidth - 8;
    widthExtension = (ext->dataWidth > 24) ? 1: 0;
    channel1Bits = (widthExtension << 15) | (ext->ch1Pos << 4) | widthBits; // start @ clk 1 of frame
    channel2Bits = channel1Bits | (32 << 4); // start 32 bits after ch1 (assuming SCLK/LRCLK == 64)
    //enable channels
    channel1Bits |= (1 << 14);
    channel2Bits |= (ch2Enable << 14);
    txrxInitBits = (channel1Bits << 16) | channel2Bits;
    pcmMap[PCM_RXC_REG] = txrxInitBits;
    pcmMap[PCM_TXC_REG] = txrxInitBits;
    DEBUG_REG("RX reg", pcmMap[PCM_RXC_REG]);
}

/*
mode = 0 --> polled mode
mode = 1 --> interrupt mode
mode = 2 --> DMA mode
*/
void initPCM(pcmExternInterface * ext, unsigned char thresh, char mode) {
    if (!pcmMap) {
        if(!(pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE)))
            return;
        // set all PCM GPIO regs to their ALT0 func
        for (int i = 18; i <= 21; i++) {
            setPinMode(i, 4);
            if (DEBUG) printf("pin %d set to ALT0\n", i);
        }
    }
    if (pcmRunning) {
        printf("ERROR: PCM interface is currently running.\nAborting...\n");
        return;
    }
    if (!checkInitParams(ext, thresh, mode)) {
        printf("Aborting...\n");
        return;
    }
    printf("Initializing PCM interface...");
    pcmMap[PCM_CTRL_REG] = (pcmMap[PCM_CTRL_REG] & CLEAR_CTRL_BITS) + 1; // clear register and set enable bit
    DEBUG_REG("Control reg right after enable set", pcmMap[PCM_CTRL_REG]);
    // CLKM == FSM
    pcmMap[PCM_MODE_REG] = ((ext->isMasterDevice << 23) | (!ext->inputOnFallingEdge << 22) | (ext->isMasterDevice << 21) | (ext->frameLength << 10) | ext->frameLength);
    initRXTXControlRegisters(ext);
    // assert RXCLR & TXCLR, wait 2 PCM clk cycles
    pcmMap[PCM_CTRL_REG] |= TXCLR | RXCLR;    
    syncWait = getSyncDelay();
    switch(mode) {
        case 1: // interrupt
        {
            pcmMap[PCM_CTRL_REG] |= (thresh << 7) | (thresh << 5);
            pcmMap[PCM_INTEN_REG] |= 3; // enable interrupts
            break;
        }
        case 2: // DMA // TODO
        {
            pcmMap[PCM_CTRL_REG] |= (1 << 9); // DMAEN
            pcmMap[PCM_DREQ_REG] |= (thresh << 8) | thresh;
            // TODO: dma.h
            break;
        }
        default: // polling
        {
            pcmMap[PCM_CTRL_REG] |= (thresh << 7) | (thresh << 5); 
            break;
        }
    }
    DEBUG_REG("Mode reg at end of init", pcmMap[PCM_MODE_REG]);
    DEBUG_REG("Control reg at end of init", pcmMap[PCM_CTRL_REG]);
    pcmMode = mode;
    pcmInitialized = 1;
    printf("done.\n");
}

void RXTest() {
    if (!pcmInitialized) {
        printf("ERROR: PCM interface has not been initialized yet.\n");
        return;
    }
    pcmMap[PCM_CTRL_REG] &= TXOFF;
    pcmMap[PCM_CTRL_REG] |= RXON; 
    while(1) {
        printf("%d\n", pcmMap[PCM_FIFO_REG]);
    }
}

void TXTest() {
    if (!pcmInitialized) {
        printf("ERROR: PCM interface has not been initialized yet.\n");
        return;
    }
    int data = 0x55555555;
    pcmMap[PCM_FIFO_REG] = data;
    pcmMap[PCM_CTRL_REG] &= RXOFF;
    pcmMap[PCM_CTRL_REG] |= TXON;
    while (1) {
        while(!TXEMPTY);
        pcmMap[PCM_FIFO_REG] = data;
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
void startPCM() {
    if (!pcmInitialized) {
        printf("ERROR: PCM interface has not been initialized yet.\n");
        return;
    }
    int data;
    char rxtxOn;
    pcmRunning = 1;
    rxtxOn = 3;
    pcmMap[PCM_CTRL_REG] &= TXOFF;
    pcmMap[PCM_CTRL_REG] |= RXON;
    switch(pcmMode) {
        case 1: { // interrupt
            break;
        }
        case 2: { // DMA
            break;
        }
        default: { // polling
            while(1) {
                while(!RXFULL);
                if (DEBUG) printf("receiving...\n");
                data = pcmMap[PCM_FIFO_REG];
                pcmMap[PCM_CTRL_REG] &= RXOFF;
                pcmMap[PCM_CTRL_REG] |= TXON;
                while(!TXEMPTY);
                if (DEBUG) printf("transmitting...\n");
                pcmMap[PCM_FIFO_REG] = data;
                pcmMap[PCM_CTRL_REG] &= TXOFF;
                pcmMap[PCM_CTRL_REG] |= RXON;               
                /*if (RXFULL & TXEMPTY) {
                    pcmMap[PCM_CTRL_REG] = pcmMap[PCM_CTRL_REG] & RXOFF | TXON;
                    pcmMap[PCM_FIFO_REG] = data;
                } else if (RXFULL) {
                    pcmMap[PCM_CTRL_REG] &= RXOFF;
                } else if (TXEMPTY) {
                    pcmMap[PCM_CTRL_REG] |= TXON;
                } else {
                    pcmMap[PCM_CTRL_REG] |= pcmMap[PCM_CTRL_REG] & TXOFF | RXON;
                    data = pcmMap[PCM_FIFO_REG];
                }*/    
                /*switch(rxtxOn) {
                    case 1: { // rx off, tx on
                        if (RXFULL & TXEMPTY) {
                            
                        } else if (RXFULL) {
                        
                        } else if (TXEMPTY) {
                        
                        }
                        break;
                    }
                    case 2: { // rx on, tx off
                        break;
                    }
                    case 3: { // both on
                        break;
                    }
                    default: { // both off
                        break;
                    }
                }*/
            }
            break;
        }
    }
}

#include "pcm.h"

static unsigned * pcmMap; 
static unsigned * dmaMap;
static bool pcmInitialized;
static bool pcmRunning;

static char pcmMode;
static int syncWait;

static DMAControlBlock * rxCtrlBlk, txCtrlBlk;

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

static void initRXTXControlRegisters(pcmExternInterface * ext, bool packedMode) {
    char widthBits, widthExtension, ch2Enable;
    unsigned short channel1Bits, channel2Bits;   
    int txrxInitBits;
    ch2Enable = 0;
    if (ext->numChannels==2) { 
        ch2Enable = 1;
        // truncate to 16b data if necessary
        if (packedMode && ext->dataWidth > 16) {
            printf("\nTruncating channel width to 16 bits...\n");
            ext->dataWidth = 16;
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


static void initDMAMode(unsigned char thresh, bool packedMode) {
    int * rxtxRAMPtr;
    if (!dmaMap)
        dmaMap = initDMAMap(2);

    // ensure that control block is 256bit address aligned
    rxCtrlBlk = (DMAControlBlock*)getAlignedPointer((void*)rxCtrlBlk, sizeof(*rxCtrlBlk));
    txCtrlBlk = (DMAControlBlock*)getAlignedPointer((void*)txCtrlBlk, sizeof(*txCtrlBlk));

    // peripheral addresses must be physical
    int bcm_base = getPhysAddrBase();
    int fifoPhysAddr = bcm_base + PCM_BASE_OFFSET + (PCM_FIFO_REG<<2);
    if (DEBUG) printf("FIFO physical address = %x\n", fifoPhysAddr);

    // set src and dest as well as DREQ signals
    rxCtrlBlk -> srcAddr = fifoPhysAddr;
    rxCtrlBlk -> destAddr = rxtxRAMPtr;
    rxCtrlBlk -> transferInfo = PERMAP(RXPERMAP) | SRC_DREQ;
    txCtrlBlk -> srcAddr = rxtxRAMPtr;
    txCtrlBlk -> destAddr = fifoPhysAddr;
    txCtrlBlk -> transferInfo = PERMAP(TXPERMAP) | DEST_DREQ;

    // TODO: verify transfer length line
    // if using packed mode, then a single data transfer is 2-channel, therefore twice the data width
    rxCtrlBlk -> transferLength = packedMode ? (ext->dataWidth) >> 1 : (ext->dataWidth) >> 2; // represented in bytes
    txCtrlBlk -> transferLength = rxCtrlBlk -> transferLength;

    // set both to run infinitely by fetching itself after operation complete
    rxCtrlBlk -> nextControlBlockAddr = rxCtrlBlk;
    txCtrlBlk -> nextControlBlockAddr = txCtrlBlk;
    DEBUG_VAL("size of control block", sizeof(*rxCtrlBlk));
    DEBUG_CTRL_BLK("rx ctrl block", rxCtrlBlk);
    DEBUG_CTRL_BLK("tx ctrl block", txCtrlBlk);

    // set up one DMA channel for RX, one for TX
    dmaMap[DMA_CONBLK_AD_REG(0)] = (int)rxCtrlBlk;
    dmaMap[DMA_CONBLK_AD_REG(1)] = (int)txCtrlBlk;

    // TODO: make this dynamic
    pcmMap[PCM_DREQ_REG] = ((thresh + 1) << 24) | ((thresh + 1)<<16) | (thresh << 8) | thresh;
    pcmMap[PCM_CTRL_REG] |= (1 << 9); // DMAEN
}


// TODO: add params to determine if packedMode
/*

ext: pointer to a struct that defines the settings of the off-board device for PCM and how our code should interact with it

thresh: threshold for TX and RX registers. Interpretation depends on mode
    for poll mode:
    for interrupt mode:
    for DMA mode:
        value of TX & RX request level (in DREQ_A reg). When the FIFO data level is above this the PCM will assert its DMA DREQ
        signal to request that some more data is read out of the FIFO

mode:
    0 --> polled mode
    1 --> interrupt mode
    2 --> DMA mode

packedMode: (for 2 channel data)
    1: use packed mode, where each word to FIFO reg is two 16b signals together, 1 representing each channel

cb: pointer to control block for DMA mode. This input is ignored for polled and interrupt mode
*/
void initPCM(pcmExternInterface * ext, unsigned char thresh, char mode, bool packedMode) {
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
    if (!checkInitParams(ext, thresh, mode, cb)) {
        printf("Aborting...\n");
        return;
    }
    if (packedMode & ext->numChannels != 2)
        packedMode = 0;
    printf("Initializing PCM interface...");

    // CLKM == FSM
    pcmMap[PCM_MODE_REG] = (3*packedMode << 24) | ((ext->isMasterDevice << 23) | (!ext->inputOnFallingEdge << 22) | (ext->isMasterDevice << 21) | (ext->frameLength << 10) | ext->frameLength);
    DEBUG_REG("Mode reg", pcmMap[PCM_MODE_REG]);

    initRXTXControlRegisters(ext, packedMode);

    // assert RXCLR & TXCLR, wait 2 PCM clk cycles
    pcmMap[PCM_CTRL_REG] = TXCLR | RXCLR;    
    syncWait = getSyncDelay();

    // RAMs should be released from standby before receive/transmit ops
    pcmMap[PCM_CTRL_REG] |= STBY;

    switch(mode) {
        case INTERRUPT_MODE:
        {
            pcmMap[PCM_CTRL_REG] |= (thresh << 7) | (thresh << 5);
            pcmMap[PCM_INTEN_REG] |= 3; // enable interrupts
            break;
        }
        case DMA_MODE:
        {
            initDMAMode(thresh, packedMode);
            break;
        }
        default: // polling
        {
            pcmMap[PCM_CTRL_REG] |= (thresh << 7) | (thresh << 5); 
            break;
        }
    }

    pcmMap[PCM_CTRL_REG] |= 1; // enable PCM
    DEBUG_REG("Mode reg at end of init", pcmMap[PCM_MODE_REG]);
    DEBUG_REG("Control reg at end of init", pcmMap[PCM_CTRL_REG]);
    pcmMode = mode;
    pcmInitialized = 1;
    printf("done.\n");
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
    pcmRunning = 1;
    // NOTE: transmit FIFO should be pre-loaded with data
    switch(pcmMode) {
        case INTERRUPT_MODE: { 
            break;
        }
        case DMA_MODE: { 
            // start the DMA (which should fill the TX FIFO)
            dmaMap[DMA_CS_REG(0)] |= 1;
            dmaMap[DMA_CS_REG(1)] |= 1;
            DEBUG_REG("Control reg after DMA enabled", pcmMap[PCM_CTRL_REG]);
            break;
        }
        default: { // polling
            break;
        }
    }

    pcmMap[PCM_CTRL_REG] |= RXONTXON;
}

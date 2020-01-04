#include "pcm.h"

static unsigned * pcmMap; 
static unsigned * dmaMap;
static bool pcmInitialized;
static bool pcmRunning;

static int syncWait;

static DMAControlBlock * rxCtrlBlk;
static DMAControlBlock * txCtrlBlk;
static unsigned * rxtxRAMptr;

// TODO
static bool checkFrameAndChannelWidth(pcmExternInterface * ext) {
    return 0;
}

static bool checkInitParams(pcmExternInterface * ext, unsigned char thresh) {
    bool error = 0;
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
    if (thresh >= 128) {
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
    pcmMap[PCM_CTRL_REG] ^= 1<<24;
    // time
    gettimeofday(&initTime, NULL);
    while ((pcmMap[PCM_CTRL_REG] >> 24) ^ notSync); // wait until values are the same of each other (this will equate to 0)
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


static void initDMAMode(char dataWidth, unsigned char thresh, bool packedMode) {
    int rxtxRAM = 0;
    rxtxRAMptr = &rxtxRAM;
    if (!dmaMap)
        dmaMap = initDMAMap(2);
    DMAControlBlock rxCtrlDummy, txCtrlDummy;

    // set DMAEN to enable DREQ generation and set RX/TXREQ, RX/TXPANIC
    pcmMap[PCM_CTRL_REG] |= (1 << 9); // DMAEN
    
    usleep(1000);
    
    pcmMap[PCM_DREQ_REG] = ((thresh + 1) << 24) | ((thresh + 1)<<16) | (thresh << 8) | thresh;
    
    // peripheral addresses must be physical
    int bcm_base = getPhysAddrBase();
    unsigned fifoPhysAddr = bcm_base + PCM_BASE_OFFSET + (PCM_FIFO_REG<<2);
    if (DEBUG) printf("FIFO physical address = %x\n", fifoPhysAddr);
    
    // set src and dest as well as DREQ signals
    rxCtrlDummy.srcAddr = fifoPhysAddr;
    rxCtrlDummy.destAddr = (unsigned)rxtxRAMptr;
    rxCtrlDummy.transferInfo = PERMAP(RXPERMAP) | SRC_DREQ;
    txCtrlDummy.srcAddr = rxCtrlDummy.destAddr;
    txCtrlDummy.destAddr = rxCtrlDummy.srcAddr;
    txCtrlDummy.transferInfo = PERMAP(TXPERMAP) | DEST_DREQ;
    
    // TODO: verify transfer length line
    // if using packed mode, then a single data transfer is 2-channel, therefore twice the data width
    rxCtrlDummy.transferLength = packedMode ? dataWidth >> 1 : dataWidth >> 2; // represented in bytes
    txCtrlDummy.transferLength = rxCtrlDummy.transferLength;
    
    // initialize ptrs, then 256b-align if necessary
    rxCtrlBlk = &rxCtrlDummy;
    txCtrlBlk = &txCtrlDummy;
    rxCtrlBlk = (DMAControlBlock*)getAlignedPointer((void*)rxCtrlBlk, sizeof(*rxCtrlBlk));
    txCtrlBlk = (DMAControlBlock*)getAlignedPointer((void*)txCtrlBlk, sizeof(*txCtrlBlk));
    *rxCtrlBlk = rxCtrlDummy;
    *txCtrlBlk = txCtrlDummy;
    
    rxCtrlBlk -> stride = 0;
    rxCtrlBlk -> reserved[1] = 0;
    rxCtrlBlk -> reserved[0] = 0;
    txCtrlBlk -> stride = 0;
    txCtrlBlk -> reserved[1] = 0;
    txCtrlBlk -> reserved[0] = 0;
    
    if (DEBUG) printf("rx ctrl blk address = %p\ntx ctrl blk address = %p\n", rxCtrlBlk, txCtrlBlk);
    
    VERBOSE_MSG("Control blocks set.\n");
    
    // set both to run infinitely by fetching itself after operation complete
    //rxCtrlBlk -> nextControlBlockAddr = (unsigned)rxCtrlBlk;
    //txCtrlBlk -> nextControlBlockAddr = (unsigned)txCtrlBlk;

    rxCtrlBlk -> nextControlBlockAddr = (unsigned)txCtrlBlk;
    txCtrlBlk -> nextControlBlockAddr = (unsigned)rxCtrlBlk;

    DEBUG_VAL("size of control block", sizeof(*rxCtrlBlk));
    
    VERBOSE_MSG("Control block loop(s) set.\n");
    
    // doesn't work ... could try using debug reg in DMA
    /*DEBUG_CTRL_BLK("rx ctrl block", rxCtrlBlk);
    DEBUG_CTRL_BLK("tx ctrl block", txCtrlBlk);*/
    if (DEBUG) {
        printf("RX:\n\ttransfer info = %x\n\tsrc address = %x\n\tdest address = %x\n\ttransfer length = %d\n\tstride = %d\n\tnext blk = %x\n\treserved[1] = %d\n\treserved[0] = %d\n",
                rxCtrlBlk -> transferInfo, rxCtrlBlk -> srcAddr, rxCtrlBlk -> destAddr, rxCtrlBlk -> transferLength, rxCtrlBlk -> stride, rxCtrlBlk -> nextControlBlockAddr, rxCtrlBlk->reserved[1],
                rxCtrlBlk -> reserved[0]);
    }

    // set up one DMA channel for RX, one for TX
    dmaMap[DMA_CONBLK_AD_REG(0)] = (int)rxCtrlBlk;
    //dmaMap[DMA_CONBLK_AD_REG(1)] = (int)txCtrlBlk;

    VERBOSE_MSG("Control blocks loaded into DMA registers.\nDMA mode successfully initialized.\n");
}


// TODO: add params to determine if packedMode
/*

ext: pointer to a struct that defines the settings of the off-board device for PCM and how our code should interact with it

thresh: threshold for TX and RX registers. Interpretation depends on mode
    for DMA mode:
        value of TX & RX request levelext (in DREQ_A reg). When the FIFO data level is above this the PCM will assert its DMA DREQ
        signal to request that some more data is read out of the FIFO

packedMode: (for 2 channel data)
    1: use packed mode, where each word to FIFO reg is two 16b signals together, 1 representing each channel
*/
void initPCM(pcmExternInterface * ext, unsigned char thresh, bool packedMode) {
    if (!pcmMap) {
        if(!(pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE)))
            return;
    }
    if (pcmRunning) {
        printf("ERROR: PCM interface is currently running.\nAborting...\n");
        return;
    }
    if (!checkInitParams(ext, thresh)) {
        printf("Aborting...\n");
        return;
    }
    if (packedMode & ext->numChannels != 2)
        packedMode = 0;
    printf("Initializing PCM interface...");

    // enable PCM
    VERBOSE_MSG("Enabling PCM...\n");
    pcmMap[PCM_CTRL_REG] = 1; 

    usleep(1000);

    // set all operational values to define frame and channel settings
    // CLKM == FSM
    pcmMap[PCM_MODE_REG] = (3*packedMode << 24) | ((ext->isMasterDevice << 23) | (!ext->inputOnFallingEdge << 22) | (ext->isMasterDevice << 21) | (ext->frameLength << 10) | ext->frameLength);
    
    usleep(1000);
    
    DEBUG_REG("Mode reg", pcmMap[PCM_MODE_REG]);
    initRXTXControlRegisters(ext, packedMode);
    
    usleep(1000);

    // assert RXCLR & TXCLR, wait 2 PCM clk cycles
    pcmMap[PCM_CTRL_REG] |= TXCLR | RXCLR;    
    usleep(1000);//syncWait = getSyncDelay();

    // RAMs should be released from standby before receive/transmit ops
    pcmMap[PCM_CTRL_REG] |= STBY;
    usleep(2000);//syncWait*2); // allow for at least 4 pcm clock cycles after clearing

    initDMAMode(ext->dataWidth, thresh, packedMode);

    DEBUG_REG("Mode reg at end of init", pcmMap[PCM_MODE_REG]);
    DEBUG_REG("Control reg at end of init", pcmMap[PCM_CTRL_REG]);
    
    for (int i = 18; i <= 21; i++) {
        setPinMode(i, 4);
        if (DEBUG) printf("pin %d set to ALT0\n", i);
    }
    
    pcmInitialized = 1;
    printf("done.\n");

}

// based on initPCM method from --> https://github.com/joan2937/pigpio/blob/master/pigpio.c
void initPCMpigpio(pcmExternInterface * ext, unsigned char thresh, bool packedMode) {
    if (!pcmMap) {
        if(!(pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE)))
            return;
    }
    
    pcmMap[PCM_CTRL_REG] = 0;

    usleep(1000);

    pcmMap[PCM_FIFO_REG] = 0;
    pcmMap[PCM_MODE_REG] = 0;
    pcmMap[PCM_RXC_REG] = 0;
    pcmMap[PCM_TXC_REG] = 0;
    pcmMap[PCM_DREQ_REG] = 0;
    pcmMap[PCM_INTEN_REG] = 0;
    pcmMap[PCM_INTSTC_REG] = 0;
    pcmMap[PCM_GRAY_REG] = 0;

    usleep(1000);

    pcmMap[PCM_MODE_REG] = 0x3A00000;
    pcmMap[PCM_TXC_REG] = 0x40184218;
    pcmMap[PCM_CTRL_REG] |= STBY; 

    usleep(1000);

    pcmMap[PCM_CTRL_REG] |= TXCLR | RXCLR;

    initDMAMode(ext->dataWidth, thresh, packedMode);

    pcmMap[PCM_INTSTC_REG] = 0b1111;

    pcmMap[PCM_CTRL_REG] |= 1; // enable PCM

    for (int i = 18; i <= 21; i++) {
        setPinMode(i, 4);
        if (DEBUG) printf("pin %d set to ALT0\n", i);
    }

    pcmInitialized = 1;
    pcmRunning = 1;

    // start dma controllers
    //dmaMap[DMA_CS_REG(1)] |= 1;
    dmaMap[DMA_CS_REG(0)] |= 1;

    pcmMap[PCM_CTRL_REG] |= RXONTXON;
}

void startPCM() {
    if (!pcmInitialized) {
        printf("ERROR: PCM interface has not been initialized yet.\n");
        return;
    }
    pcmRunning = 1;
    VERBOSE_MSG("Starting PCM...\n");
    // NOTE: transmit FIFO should be pre-loaded with data
    // start the DMA (which should fill the TX FIFO)
    dmaMap[DMA_CS_REG(1)] |= 1;
    
    usleep(1000);
    
    pcmMap[PCM_CTRL_REG] |= TXON;
    
    usleep(1000);
    
    dmaMap[DMA_CS_REG(0)] |= 1;
    
    usleep(1000);
    
    DEBUG_REG("Control reg after DMA enabled", pcmMap[PCM_CTRL_REG]);
    VERBOSE_MSG("Running...\n");

    pcmMap[PCM_CTRL_REG] |= RXONTXON;
}

#include "pcm.h"

static volatile uint32_t * pcmMap = 0;
static bool pcmInitialized;
static bool pcmRunning;

static int32_t syncWait;

static VirtToBusPages bufferPages;

//static DMAControlBlock * rxCtrlBlk;
//static DMAControlBlock * txCtrlBlk;
//static unsigned * rxtxRAMptr;

// TODO
static bool checkFrameAndChannelWidth(pcmExternInterface * ext) {
    return 0;
}

static void checkInitParams(pcmExternInterface * ext, uint8_t thresh, uint8_t panicThresh) {
    if (!ext->isMasterDevice && !checkFrameAndChannelWidth(ext))
        FATAL_ERROR("incompatible frame lengths and data widths.")
    if (ext->numChannels < 1 || ext->numChannels > 2) 
        FATAL_ERROR("valid number of channels are 1 or 2.");
    if (ext->dataWidth > 32) {
        FATAL_ERROR("maximum data width available is 32 bits.");
    } else if (ext->dataWidth % 8) {
        FATAL_ERROR("please set data width to be a multiple of 8 bits.");
    }
    if (thresh >= 128 || panicThresh >= 128)
        FATAL_ERROR("thresholds must be six-bit values for DMA mode.")
}

/*
determine the delay (in microseconds) between writing to SYNC in PCM CTRL reg
and reading back the value written. This corresponds to ~2 PCM clk cycles

should only need to call once to get an idea of how long to delay the program
*/ 
static int32_t getSyncDelay() {
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
    uint32_t usElapsed = ((endTime.tv_sec - initTime.tv_sec)*1000000) + (endTime.tv_usec - initTime.tv_usec);
    DEBUG_VAL("2-cycle PCM clk delay in micros", (usElapsed + (10 - (usElapsed % 10))));
    // round up
    return usElapsed + (10 - (usElapsed % 10));
}

static void initRXTXControlRegisters(pcmExternInterface * ext, bool packedMode) {
    uint8_t widthBits, widthExtension, ch2Enable;
    uint16_t channel1Bits, channel2Bits;   
    uint32_t txrxInitBits;

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


static void initDMAMode(uint8_t dataWidth, uint8_t thresh, uint8_t panicThresh, bool packedMode) {
    // set DMAEN to enable DREQ generation and set RX/TXREQ, RX/TXPANIC
    pcmMap[PCM_CTRL_REG] |= (1 << 9); // DMAEN
    
    usleep(1000);

    // TODO: separate panic thresholds from dreq thresholds
    pcmMap[PCM_DREQ_REG] = (panicThresh << 24) | (panicThresh<<16) | (thresh << 8) | thresh;

    // read from FIFO to here, and write from here to FIFO
    bufferPages = initUncachedMemView(BCM_PAGESIZE, USE_DIRECT_UNCACHED);
    void * virtBufferPages = bufferPages.virtAddr;
    void * busBufferPages = (void *)bufferPages.busAddr;
    
    // peripheral addresses must be physical
    //uint32_t bcm_base = getBCMBase();
    uint32_t fifoPhysAddr = BUS_BASE + PCM_BASE_OFFSET + (PCM_FIFO_REG<<2); // PCM_FIFO_REG is macro defined by int offset, need byte offset
    uint32_t csPhysAddr = BUS_BASE + PCM_BASE_OFFSET + (PCM_CTRL_REG<<2);

    if (DEBUG) printf("FIFO physical address = %x\n", fifoPhysAddr);

    // these bytes are the sources for two DMA control blocks, whose destinations are the PCM_CTRL_REG. This way the DMA can automatically
    // update TXON and RXON each time it switches between reading from the FIFO and writing to the FIFO
    uint8_t dmaRxOnByte = (uint8_t)(pcmMap[PCM_CTRL_REG] & 0xF9) | RXON;
    uint8_t dmaTxOnByte = (uint8_t)(pcmMap[PCM_CTRL_REG] & 0xF9) | TXON;

    uint32_t lastByteIdx = sizeof(virtBufferPages) - 1;

    ((char *)virtBufferPages)[lastByteIdx - 1] = dmaRxOnByte;
    ((char *)virtBufferPages)[lastByteIdx] = dmaTxOnByte;
    
    // TODO: verify transfer length line
    // if using packed mode, then a single data transfer is 2-channel, therefore twice the data width
    uint32_t transferLength = packedMode ? dataWidth >> 1 : dataWidth >> 2; // represented in bytes
    
    uint32_t rxTransferInfo = (PERMAP(RXPERMAP) | SRC_DREQ | SRC_INC | DEST_INC);
    uint32_t txTransferInfo = (PERMAP(TXPERMAP) | DEST_DREQ | SRC_INC | DEST_INC);

    DMAControlPageWrapper cbWrapper = initDMAControlPage(4);

    /*
    4 DMA Control blocks:

    CB0: write from buffer pages to FIFO (when TXDREQ high)
    CB1: set TX OFF and RX ON in the PCM CTRL REG
    CB2: read from FIFO to buffer pages (when RXDREQ high)
    CB3: set TX ON and RX OFF in the PCM CTRL REG

    (repeat)
    */

    // for now, insertDMAControlBlock() is an internally inconsistent function, and I would not use it.
    /*insertDMAControlBlock (&cbWrapper, txTransferInfo, (uint32_t)busBufferPages, fifoPhysAddr, transferLength, 0);
    insertDMAControlBlock (&cbWrapper, 0, (uint32_t)&(((char *)busBufferPages)[lastByteIdx-1]), csPhysAddr, 1, 1);
    insertDMAControlBlock (&cbWrapper, rxTransferInfo, fifoPhysAddr, (uint32_t)busBufferPages, transferLength, 2);
    insertDMAControlBlock (&cbWrapper, 0, (uint32_t)&(((char *)busBufferPages)[lastByteIdx]), csPhysAddr, 1, 3);*/
    
    initDMAControlBlock(&cbWrapper, txTransferInfo, (uint32_t)busBufferPages, fifoPhysAddr, transferLength);
    initDMAControlBlock(&cbWrapper, 0, (uint32_t)&(((char *)busBufferPages)[lastByteIdx-1]), csPhysAddr, 1);
    initDMAControlBlock(&cbWrapper, rxTransferInfo, fifoPhysAddr, (uint32_t)busBufferPages, transferLength);
    initDMAControlBlock(&cbWrapper, 0, (uint32_t)&(((char *)busBufferPages)[lastByteIdx]), csPhysAddr, 1);
    
    VERBOSE_MSG("Control blocks set.\n");
    
    // create loop
    linkDMAControlBlocks(&cbWrapper, 0, 1);
    linkDMAControlBlocks(&cbWrapper, 1, 2);
    linkDMAControlBlocks(&cbWrapper, 2, 3);    
    linkDMAControlBlocks(&cbWrapper, 3, 0);
    VERBOSE_MSG("Control block loop(s) set.\n");

    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), 5);
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
void initPCM(pcmExternInterface * ext, uint8_t thresh, uint8_t panicThresh, bool packedMode) {
    if (!pcmMap) {
        if(!(pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE)))
            return;
    }
    if (pcmRunning) {
        printf("ERROR: PCM interface is currently running.\nAborting...\n");
        return;
    }
    
    checkInitParams(ext, thresh, panicThresh);
    
    if (packedMode && ext->numChannels != 2)
        packedMode = 0;
    printf("Initializing PCM interface...");

    // enable PCM
    VERBOSE_MSG("Enabling PCM...\n");
    pcmMap[PCM_CTRL_REG] = 1; 

    usleep(1000);

    // set all operational values to define frame and channel settings
    // CLKM == FSM
    uint8_t flen = ext->frameLength >> 1;
    pcmMap[PCM_MODE_REG] = (3*packedMode << 24) | (ext->isMasterDevice << 23) | (!ext->inputOnFallingEdge << 22) | (ext->isMasterDevice << 21) | (flen << 10) | flen;
    
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

    initDMAMode(ext->dataWidth, thresh, panicThresh, packedMode);

    DEBUG_REG("Mode reg at end of init", pcmMap[PCM_MODE_REG]);
    DEBUG_REG("Control reg at end of init", pcmMap[PCM_CTRL_REG]);
    
    for (int i = 18; i <= 21; i++) {
        setPinMode(i, 4);
        if (DEBUG) printf("pin %d set to ALT0\n", i);
    }
    
    uint32_t mClkFreq, pcmClkFreq;
    pcmClkFreq = (uint32_t)ext->sampleFreq * ext->frameLength;
    mClkFreq = ext->isDoubleSpeedMode ? (pcmClkFreq << 2): (pcmClkFreq << 1);

    // initialize clock(s)
    initClock(0, mClkFreq, 1, PLLD);
    if (!ext->isMasterDevice)
        initClock(PCM_CLK_CTL_IDX, pcmClkFreq, 1, OSC); // can both sources use PLLD?

    pcmInitialized = 1;
    printf("done.\n");

}

void startPCM() {
    if (!pcmInitialized)
        FATAL_ERROR("PCM interface was not initialized.")
    pcmRunning = 1;
    VERBOSE_MSG("Starting PCM...\n");

    // start clock(s)
    startClock(0);
    if (~(pcmMap[PCM_MODE_REG] >> 23) & 1) // if PCM is master mode, need to start PCM CLK
        startClock(PCM_CLK_CTL_IDX);

    startDMAChannel((uint8_t)RXDMA);
    pcmMap[PCM_CTRL_REG] |= TXON;
    
    DEBUG_REG("PCM ctrl reg w/ tx and rx on", pcmMap[PCM_CTRL_REG]);
    VERBOSE_MSG("Running...\n");
}

void freePCM() {
    clearMemMap(pcmMap, PCM_BASE_MAPSIZE);
    clearUncachedMemView(&bufferPages);
}

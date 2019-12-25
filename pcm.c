#include "pcm.h"

static unsigned * pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE);

static char pcmMode;
static int syncWait;

static void checkFrameAndChannelWidth(char frameLength, char dataWidth) {
    return 0;
}

// determine the delay (in microseconds) between writing to SYNC in PCM CTRL reg
// and reading back the value written. This corresponds to ~2 PCM clk cycles
// should only need to call once to get an idea of how long to delay the program
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

/*
- set TXTHR/RXTHR
- if transmitting, ensure sufficient sample words have been written to PCMFIFO before
transmission starts.
- set TXON and RXON to begin operation
- poll TXW when writing sample words from FIFO and RXR when reading sample words until all
data is transferred
*/
static void initPolledMode() {

}

/*
- set TXTHR/RXTHR
- set INTR and/or INTT to enable interrupts
- if transmitting, ensure sufficient sample words have been written to PCMFIFO before transmission
starts.
- set TXON and RXON to begin operation
- when an interrupt occurs, check RXR. If this is set, one or more sample words are available in the FIFO.
If TXW is set then one or more sample words can be sent to the FIFO
*/
static void initInterruptMode() {

}

/*
- set DMAEN to enable DMA DREQ generation. Set RXREQ/TXREQ to determine FIFO thresholds 
for DREQs. If required, set TXPANIC and RXPANIC
- in DMA controllers set correct DREQ channels, 1 for RX, 1 for TX. Start the DMA
- set TXON and RXON to begin operation
*/ 
static void initDMAMode() {

}

/*
mode = 0 --> polled mode
mode = 1 --> interrupt mode
mode = 2 --> DMA mode

For all three modes:
  - set enable in PCM block
  - set all operational values to define frame and channel settings
  - assert RXCLR & TXCLR, wait 2 PCM clks for FIFO to reset (use SYNC to check)

clockMode: 
    0 = the PCM clk is an output and drives at the MCLK rate
    1 = the PCM clk is an input

frameSyncMode:
    0 = PCM_FS is an output we generate 
    1 = the PCM_FS is an input, we lock onto incoming frame sync signal

dataWidth:
    width of data per channel
*/
void initPCM(char mode, bool clockMode, bool frameSyncMode, char frameLength, char dataWidth) {
    printf("Initializing PCM interface...");
    if !(checkFrameAndChannelWidth(frameLength, dataWidth)) {
        printf("\nERROR: incompatible frame lengths and data widths.\n");
        return;
    }
    pcmMap(PCM_CTRL_REG) |= 1; // enable set
    // ...frame and channel settings...
    pcmMap(PCM_MODE_REG) |= ((clockMode << 23) | (frameSyncMode << 21) | (frameLength << 10) | frameLength);
    char widthBits = dataWidth - 8;
    char widthExtension = 0;
    if (dataWidth > 24)
        widthExtension = 1;
        widthBits -= 16;
    unsigned short channel1Bits, channel2Bits;
    channel1Bits = (widthExtension << 15) | widthBits;
    channel2Bits = channel1Bits | (dataWidth << 4); // o(ffset CH2 pos in frame
    int txrxInitBits = (channel1Bits << 16) | channel2Bits;
    pcmMap(PCM_RXC_REG) |= txrxInitBits;
    pcmMap(PCM_TXC_REG) |= txrxInitBits;
    // assert RXCLR & TXCLR, wait 2 PCM clk cycles
    pcmMap(PCM_CTRL_REG) |= TXCLR | RXCLR;
    syncWait = getSyncDelay();
    switch(mode) {
        case 1:
        {
            initInterruptMode(); break;
        }
        case 2:
        {
            initDMAMode(); break;
        }
        default:
        {
            initDMAMode(); break;
        }
    }
    pcmMode = mode;
    printf("done.\n");
}

void startPCM() {

}
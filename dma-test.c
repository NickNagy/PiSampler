#include "globals.h"
#include "bcm/pimem.h"
#include "bcm/dma.h"

#define DMA_TEST_CHANNEL 5

#define PCM_BASE_OFFSET 0x203000
#define PCM_BASE_MAPSIZE 0x24

#define TXCLR 8
#define RXCLR 16

volatile uint32_t * dmaMap = 0;

void dma_test_0() {
    printf("\nDMA TEST 0:\n\t use a single DMA control block to write 'hello world!' to the destination.\n");

    void * virtSrcPage, *busSrcPage;
    VirtToBusPages srcPages = initUncachedMemView(BCM_PAGESIZE, USE_DIRECT_UNCACHED);
    virtSrcPage = srcPages.virtAddr;
    busSrcPage = (void *)(srcPages.busAddr);

    void * virtDestPage, *busDestPage;
    VirtToBusPages destPages = initUncachedMemView(BCM_PAGESIZE, USE_DIRECT_UNCACHED);
    virtDestPage = destPages.virtAddr;
    busDestPage = (void *)(destPages.busAddr);
    
    printf("(Before transfer) destination reads: %s\n", (char*)virtDestPage);
    
    sleep(1);

    char *srcArr = (char *)virtSrcPage;
    srcArr[0] = 'h';
    srcArr[1] = 'e';
    srcArr[2] = 'l';
    srcArr[3] = 'l';
    srcArr[4] = 'o';
    srcArr[5] = ' ';
    srcArr[6] = 'w';
    srcArr[7] = 'o';
    srcArr[8] = 'r';
    srcArr[9] = 'l';
    srcArr[10] = 'd';
    srcArr[11] = '!';
    srcArr[12] = 0;

    DMAControlPageWrapper cbWrapper = initDMAControlPage(1);
    initDMAControlBlock(&cbWrapper, SRC_INC | DEST_INC, (uint32_t)busSrcPage, (uint32_t)busDestPage, 13);
    
    //DEBUG_PTR("start control block src addr", (uint32_t)(((DMAControlBlock *)(cbWrapper.pages.virtAddr))->srcAddr));
	
    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), DMA_TEST_CHANNEL);

    startDMAChannel(DMA_TEST_CHANNEL);

    sleep(1);

    printf("(After transfer) destination reads: '%s'\n", (char*)virtDestPage);

    clearUncachedMemView(&srcPages);
    clearUncachedMemView(&destPages);
    clearDMAControlPage(&cbWrapper);
}

/*

initially, cb1's src = "uh-oh world!"

goal: set cb1 instead to transfer "hello world!" to its destination

*/
void dma_test_1() {
    printf("\nDMA TEST 1:\n\tUse 2 DMA control blocks to change the string 'uh-oh world!' to 'hello world!' and write it to the destination.\n");

    size_t srcBytes = ceilToPage(18);
    size_t destBytes = ceilToPage(13);

    void *virtSrcPage, *busSrcPage;
    VirtToBusPages srcPages = initUncachedMemView(srcBytes, USE_DIRECT_UNCACHED);
    virtSrcPage = srcPages.virtAddr;
    busSrcPage = (void *)srcPages.busAddr;

    void *virtDestPage, *busDestPage;
    VirtToBusPages destPages = initUncachedMemView(destBytes, USE_DIRECT_UNCACHED);
    virtDestPage = destPages.virtAddr;
    busDestPage = (void *)destPages.busAddr;

    printf("(Before transfer) destination reads: %s\n", (char*)virtDestPage);

    char *srcArr = (char *)virtSrcPage;
    srcArr[0] = 'u';
    srcArr[1] = 'h';
    srcArr[2] = '-';
    srcArr[3] = 'o';
    srcArr[4] = 'h';
    srcArr[5] = ' ';
    srcArr[6] = 'w';
    srcArr[7] = 'o';
    srcArr[8] = 'r';
    srcArr[9] = 'l';
    srcArr[10] = 'd';
    srcArr[11] = '!';
    srcArr[12] = 0;
    srcArr[13] = 'h';
    srcArr[14] = 'e';
    srcArr[15] = 'l';
    srcArr[16] = 'l';
    srcArr[17] = 'o';

    DMAControlPageWrapper cbWrapper = initDMAControlPage(2);

    uint32_t transferInfo = SRC_INC | DEST_INC;

    // cb1 sets srcArr[0:5] = srcArr[13:18]
    initDMAControlBlock(&cbWrapper, transferInfo, (uint32_t)((char *)busSrcPage + 13), (uint32_t)busSrcPage, 5);
    
    // cb2 then writes srcArr[0:13] to the dest
    initDMAControlBlock(&cbWrapper, transferInfo, (uint32_t)busSrcPage, (uint32_t)busDestPage, 13);

    // set cb1's nextControlBlkAddr to cb2
    linkDMAControlBlocks(&cbWrapper, 0, 1);

    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), DMA_TEST_CHANNEL);
    startDMAChannel(DMA_TEST_CHANNEL);

    sleep(1);
    printf("(After transfer) destination reads: %s\n", (char*)virtDestPage);

    clearUncachedMemView(&srcPages);
    clearUncachedMemView(&destPages);
    clearDMAControlPage(&cbWrapper);
}

/* 

cb1's nextAddr is initially cb2's, and cb2's is cb1's --> ie, infinite loop

goal: use cb2 to write 0 to cb1's nextAddr field and break the loop

NOTE: I'm not sure this function can guarantee anything? Will the DMA cbs continue to loop over and over again even after we read the destination result??

*/
void dma_test_2() {
    printf("\nDMA TEST 2:\n\tStop two DMA control blocks who point to each other from looping infinitely by writing 0 to one of their nextControlBlockAddr fields.\n");

    void *virtSrcPage, *busSrcPage, *virtDestPage, *busDestPage;
    VirtToBusPages srcPages = initUncachedMemView(13, USE_DIRECT_UNCACHED);
    VirtToBusPages destPages = initUncachedMemView(13, USE_DIRECT_UNCACHED);
    virtSrcPage = srcPages.virtAddr;
    busSrcPage = (void*)srcPages.busAddr;
    virtDestPage = destPages.virtAddr;
    busDestPage = (void *)destPages.busAddr;

    printf("(Before transfer) destination reads: %s\n", (char*)virtDestPage);

    char *srcArr = (char *)virtSrcPage;
    srcArr[0] = 'h';
    srcArr[1] = 'e';
    srcArr[2] = 'l';
    srcArr[3] = 'l';
    srcArr[4] = 'o';
    srcArr[5] = ' ';
    srcArr[6] = 'w';
    srcArr[7] = 'o';
    srcArr[8] = 'r';
    srcArr[9] = 'l';
    srcArr[10] = 'd';
    srcArr[11] = '!';
    srcArr[12] = 0;
    srcArr[13] = 0; // these are used to pass by cb2
    srcArr[14] = 0;
    srcArr[15] = 0;

    uint32_t transferInfo = SRC_INC | DEST_INC;

    DMAControlPageWrapper cbWrapper = initDMAControlPage(2);
    initDMAControlBlock(&cbWrapper, transferInfo, (uint32_t)busSrcPage, (uint32_t)busDestPage, 13);

    // 5th int of cbWrapper == cb1.nextControlBlockAddr
    initDMAControlBlock(&cbWrapper, transferInfo, (uint32_t)((char*)busSrcPage + 12), (uint32_t)((uint32_t*)(cbWrapper.pages.busAddr) + 5), 4);

    // DANGEROUSLY create the infinite loop!!
    linkDMAControlBlocks(&cbWrapper, 0, 1);
    linkDMAControlBlocks(&cbWrapper, 1, 0);

    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), DMA_TEST_CHANNEL);
    startDMAChannel(DMA_TEST_CHANNEL);

    sleep(1);

    printf("(After transfer) destination reads: %s\n", (char*)virtDestPage);

    clearUncachedMemView(&srcPages);
    clearUncachedMemView(&destPages);
    clearDMAControlPage(&cbWrapper);  
}

void dma_test_3() {
    printf("\nDMA Test 3:\n\tChange the bottom byte of a 'reg' without manipulating its other bits.\n");
    void* virtPage, *busPage;
    VirtToBusPages pages = initUncachedMemView(BCM_PAGESIZE, USE_DIRECT_UNCACHED);
    virtPage = pages.virtAddr;
    busPage = (void*)pages.busAddr;
    uint32_t* reg = (uint32_t*)virtPage;
    *reg = 0xFFFFFFFF;
    *(reg + 1) = 0;
    
    DMAControlPageWrapper cbWrapper = initDMAControlPage(1);
    
    // move byte 4 to byte 0
    initDMAControlBlock(&cbWrapper, 0, (uint32_t)((uint8_t*)busPage + 4), (uint32_t)busPage, 1);
    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), DMA_TEST_CHANNEL);
    startDMAChannel(DMA_TEST_CHANNEL);
    
    sleep(1);
    
    printf("(After transfer) reg reads: %x\n", *reg);
    
    clearUncachedMemView(&pages);
    clearDMAControlPage(&cbWrapper);
}

void dma_test_4() {
    printf("\nDMA Test 4:\n\tUpdate the bottom byte of the PCM CS_A reg.\n");
    
    void* virtPage, *busPage;
    VirtToBusPages pages = initUncachedMemView(BCM_PAGESIZE, USE_DIRECT_UNCACHED);
    virtPage = pages.virtAddr;
    busPage = (void*)pages.busAddr;
    uint8_t *virtBytes = (uint8_t*)virtPage;
    virtBytes[0] = 0;
    
    volatile uint32_t * pcmMap = initMemMap(PCM_BASE_OFFSET, PCM_BASE_MAPSIZE);
    
    pcmMap[0] = 1; // enables PCM
    
    /* bit assignments here shouldn't bear any real significance in operation --> these
     * are just the bit assignments I have set for my audio project */
    pcmMap[0] |= (1 << 25); // RAM released from standby mode
    pcmMap[0] |= (1 << 9); // DMA enable
    pcmMap[0] |= TXCLR | RXCLR;
    sleep(1);
    
    printf("PCM CS_A reg is initialized as %x\n", pcmMap[0]);
    
    // getting physical address of pcm cs reg
    uint32_t pcmCSPhysicalAddr = BUS_BASE + PCM_BASE_OFFSET;
    
    DMAControlPageWrapper cbWrapper = initDMAControlPage(1);
    initDMAControlBlock(&cbWrapper, 0, (uint32_t)busPage, pcmCSPhysicalAddr, 1);
    initDMAChannel((DMAControlBlock *)(cbWrapper.pages.busAddr), DMA_TEST_CHANNEL);
    startDMAChannel(DMA_TEST_CHANNEL);
    
    sleep(1);
    
    printf("(After transfer) PCM CS_A reg reads %x\n", pcmMap[0]);
    
    clearUncachedMemView(&pages);
    clearDMAControlPage(&cbWrapper);
    clearMemMap((void*)pcmMap, PCM_BASE_MAPSIZE);
}

int main(int argc, char ** argv) {
    dmaMap = initMemMap(DMA_BASE_OFFSET, DMA_MAPSIZE);

    if (argc > 1) {
        switch(atoi(argv[1])) {
            case 1: {
                dma_test_1();
                break;
            }
            case 2: {
                dma_test_2();
                break;
            }
            case 3: {
                dma_test_3();
                break;
            }
            case 4: {
                dma_test_4();
                break;
            }
            default: {
                dma_test_0();
                break;
            }
        }
    } else {
        dma_test_0();
        dma_test_1();
        dma_test_2();
        dma_test_3();
        dma_test_4();
    }

    munmap((void *)dmaMap, DMA_MAPSIZE);
    
    closeFiles();

    return 0;
}

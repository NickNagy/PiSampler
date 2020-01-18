#include "globals.h"
#include "bcm/pimem.h"
#include "bcm/dma.h"

#define DMA_TEST_CHANNEL 5

volatile uint32_t * dmaMap = 0;

void addressSwapTest() {
    void * virtPtr = malloc(1);
    printf("Virtual address of ptr: %p\n", virtPtr);
    void * busPtr = virtToUncachedPhys(virtPtr, USE_DIRECT_UNCACHED);
    printf("Bus address of ptr: %p\n", busPtr);
    void * physPtr = busToPhys(busPtr);
    printf("Physical address of ptr: %p\n", physPtr);
    free (virtPtr);
}

void dma_test_wallacoloo() {
    //int dmaCh = 5;
    
    void * virtSrcPage, *physSrcPage;
    initVirtPhysPage(&virtSrcPage, &physSrcPage);

    void * virtDestPage, *physDestPage;
    initVirtPhysPage(&virtDestPage, &physDestPage);

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
	
    void * virtCBPage, *physCBPage;
    initVirtPhysPage(&virtCBPage, &physCBPage);
    
    DMAControlPageWrapper * cbWrapper = initDMAControlPage(1);
    
    DEBUG_MSG("wrapper struct init");

    DMAControlBlock * cb = (DMAControlBlock *)virtCBPage;

    cb -> transferInfo = SRC_INC | DEST_INC;
    cb -> srcAddr = (uint32_t)physSrcPage;
    cb -> destAddr = (uint32_t)physDestPage;
    cb -> transferLength = 13;
    cb -> stride = 0;
    cb -> reserved = 0;
    cb -> nextControlBlockAddr = 0;

    //enable DMA channel
	dmaMap[DMA_GLOBAL_ENABLE_REG] |= (1 << DMA_TEST_CHANNEL);

	dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL)] = DMA_RESET;
	sleep(1);

	// clear debug flags
	dmaMap[DMA_DEBUG_REG(DMA_TEST_CHANNEL)] = 7; //DMA_READ_ERROR | DMA_FIFO_ERROR | DMA_READ_LAST_NOT_SET_ERROR;
	// alternatively, can just to VirtToPhys, but probably not as efficient
	dmaMap[DMA_CONBLK_AD_REG(DMA_TEST_CHANNEL)] = (uint32_t)physCBPage; // temporarily...

    dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL)] = 1;

    sleep(1);

    printf("destination reads: '%s'\n", (char*)virtDestPage);

    clearVirtPhysPage(virtSrcPage);
    clearVirtPhysPage(virtDestPage);
    clearDMAControlPage(cbWrapper);
}

void dma_test_0() {
    printf("DMA TEST 0:\n\t use a single DMA control block to write 'hello world!' to the destination.\n");

    void * virtSrcPage, *physSrcPage;
    initVirtPhysPage(&virtSrcPage, &physSrcPage);

    void * virtDestPage, *physDestPage;
    initVirtPhysPage(&virtDestPage, &physDestPage);

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

    DMAControlPageWrapper * cbWrapper = initDMAControlPage(1);
    initDMAControlBlock(cbWrapper, SRC_INC | DEST_INC, (uint32_t)physSrcPage, (uint32_t)physDestPage, 13);
	
    initDMAChannel(cbWrapper->cbPage, DMA_TEST_CHANNEL); // replaces the code below
	/*
    //enable DMA channel
	dmaMap[DMA_GLOBAL_ENABLE_REG] |= (1 << dmaCh);

	dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL)] = DMA_RESET;
	sleep(1);
	
	// clear debug flags
	dmaMap[DMA_DEBUG_REG(DMA_TEST_CHANNEL)] = 7; //DMA_READ_ERROR | DMA_FIFO_ERROR | DMA_READ_LAST_NOT_SET_ERROR;

	// alternatively, can just to VirtToPhys, but probably not as efficient
	dmaMap[DMA_CONBLK_AD_REG(DMA_TEST_CHANNEL)] = (uint32_t)physCBPage; // temporarily...*/

    startDMAChannel(DMA_TEST_CHANNEL); //dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL] = 1; // activate DMA channel...

    sleep(1);

    printf("destination reads: '%s'\n", (char*)virtDestPage);

    clearVirtPhysPage(virtSrcPage);
    clearVirtPhysPage(virtDestPage);
    clearDMAControlPage(cbWrapper);
}

/*

initially, cb1's src = "uh-oh world!"

goal: set cb1 instead to transfer "hello world!" to its destination

*/
void dma_test_1() {
    printf("DMA TEST 1:\n\tUse 2 DMA control blocks to change the string 'uh-oh world!' to 'hello world!' and write it to the destination.\n");

    void *virtSrcPage, *virtDestPage;
    //initVirtPhysPage(&virtSrcPage, &physSrcPage);

    //void *virtDestPage, *physDestPage;
    //initVirtPhysPage(&virtDestPage, &physDestPage);

    size_t srcBytes = ceilToPage(18);
    size_t destBytes = ceilToPage(13);

    virtSrcPage = initUncachedMemView(initLockedMem(srcBytes), srcBytes, USE_DIRECT_UNCACHED);
    virtDestPage = initUncachedMemView(initLockedMem(destBytes), destBytes, USE_DIRECT_UNCACHED);

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

    DMAControlPageWrapper * cbWrapper = initDMAControlPage(2);

    uint32_t transferInfo = SRC_INC | DEST_INC;

    // cb1 sets srcArr[0:5] = srcArr[13:18]
    initDMAControlBlock(cbWrapper, transferInfo, (uint32_t)virtToUncachedPhys((void*)&(srcArr[13]), USE_DIRECT_UNCACHED), (uint32_t)virtToUncachedPhys((void*)&srcArr, USE_DIRECT_UNCACHED), 5);
    
    // cb2 then writes srcArr[0:13] to the dest
    initDMAControlBlock(cbWrapper, transferInfo, (uint32_t)virtToUncachedPhys(srcArr, USE_DIRECT_UNCACHED), (uint32_t)virtToUncachedPhys(virtDestPage, USE_DIRECT_UNCACHED), 13);

    // set cb1's nextControlBlkAddr to cb2
    linkDMAControlBlocks(cbWrapper, 0, 1);

    initDMAChannel(cbWrapper->cbPage, DMA_TEST_CHANNEL);
    startDMAChannel(DMA_TEST_CHANNEL);

    sleep(1);
    printf("destination reads: %s\n", (char*)virtDestPage);

    clearUncachedMemView(virtSrcPage, srcBytes);
    clearUncachedMemView(virtDestPage, destBytes);
    clearDMAControlPage(cbWrapper);
}

int main(int argc, char ** argv) {
    dmaMap = initMemMap(DMA_BASE_OFFSET, DMA_MAPSIZE);

    /* if (argc > 1) {
        switch((int)argv[1]) {
            case 1: {
                dma_test_1();
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
    } */

    addressSwapTest();//dma_test_wallacoloo();

    munmap((void *)dmaMap, DMA_MAPSIZE);

    return 0;
}

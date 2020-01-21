#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <bcm_host.h>
#include <errno.h>

#define ERROR_MSG(msg) printf("ERROR: %s\n", msg);
#define FATAL_ERROR(msg) {ERROR_MSG(msg); exit(1);}
#define DEBUG_PTR(name, ptr) printf("%s = %p\n", name, ptr);

#define PAGE_SIZE 4096

// Message IDs for different mailbox GPU memory allocation messages
#define MEM_ALLOC_MESSAGE 0x3000c // This message is 3 u32s: numBytes, alignment and flags
#define MEM_FREE_MESSAGE 0x3000f // This message is 1 u32: handle
#define MEM_LOCK_MESSAGE 0x3000d // 1 u32: handle
#define MEM_UNLOCK_MESSAGE 0x3000e // 1 u32: handle

// Memory allocation flags
#define MEM_ALLOC_FLAG_DIRECT (1 << 2) // Allocate uncached memory that bypasses L1 and L2 cache on loads and stores

// dma
#define DMA_BASE_OFFSET  0x007000
#define DMA_MAPSIZE      0x1000
#define DMA_SINGLE_REG_MAPSIZE 0x100

#define VC_BUS_BASE 0xC0000000

#define DMA_CHANNEL(x) x*64

#define DMA_CS_REG(x)        DMA_CHANNEL(x)
#define DMA_CONBLK_AD_REG(x) DMA_CHANNEL(x) + 1
#define DMA_TI_REG(X)        DMA_CHANNEL(x) + 2
#define DMA_SOURCE_AD_REG(x) DMA_CHANNEL(x) + 3
#define DMA_DEST_AD_REG(x)   DMA_CHANNEL(x) + 4
#define DMA_TXFR_LEN_REG(x)  DMA_CHANNEL(x) + 5
#define DMA_STRIDE_REG(x)    DMA_CHANNEL(x) + 6
#define DMA_NEXTCONBK_REG(x) DMA_CHANNEL(x) + 7
#define DMA_DEBUG_REG(x)     DMA_CHANNEL(x) + 8

#define DMA_GLOBAL_ENABLE_REG   0xFF0>>2

// Control status register bits
#define DMA_RESET 1<<31
#define DMA_ABORT 1<<30

#define SRC_INC  1<<8
#define DEST_INC 1<<4 

static uint32_t bcm_base = 0;
static uint32_t memfd = 0;
static uint32_t pagemapfd = 0;
static uint32_t vciofd = 0;

static volatile uint32_t *dmaMap = 0;

struct GpuMemory
{
  uint32_t allocationHandle;
  void *virtualAddr;
  uintptr_t busAddress;
  uint32_t sizeBytes;
};

// Defines the structure of a Mailbox message
template<int PayloadSize>
struct MailboxMessage
{
  MailboxMessage(uint32_t messageId):messageSize(sizeof(*this)), requestCode(0), messageId(messageId), messageSizeBytes(sizeof(uint32_t)*PayloadSize), dataSizeBytes(sizeof(uint32_t)*PayloadSize), messageEndSentinel(0) {}
  uint32_t messageSize;
  uint32_t requestCode;
  uint32_t messageId;
  uint32_t messageSizeBytes;
  uint32_t dataSizeBytes;
  union
  {
    uint32_t payload[PayloadSize];
    uint32_t result;
  };
  uint32_t messageEndSentinel;
};

struct DMAControlBlock {
  uint32_t transferInfo;
  uint32_t srcAddr;
  uint32_t destAddr;
  uint32_t transferLength;
  uint32_t stride;
  uint32_t nextControlBlockAddr;
  uint64_t reserved;
};

/* 
initializes memfd and pagemapfd, which are used by multiple functions in this file
returns 0 if successful, 1 otherwise
*/
static bool openFiles() {
    if (!memfd) {
        if((memfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
            ERROR_MSG("Failure to access /dev/mem."); 
            return 1;
        }
    }
    if (!pagemapfd) {
        if((pagemapfd = open("/proc/self/pagemap", 'r')) < 0) {
            ERROR_MSG("Failure to access /proc/self/pagemap.");
            return 1;
        }
    }
    if (!vciofd) {
        if ((vciofd = open("/dev/vcio", 0)) < 0) {
            ERROR_MSG("Failure to access /dev/vcio.");
            return 1;
        }
    }
    return 0;
}

void closeFiles() {
    if (memfd)
        close(memfd);
    if (pagemapfd)
        close(pagemapfd);
    if (vciofd)
        close(vciofd);
}

/* returns a map to memory of given size and @ address offset */
volatile uint32_t * initMemMap(uint32_t offset, uint32_t size) {
    if (offset % PAGE_SIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", PAGE_SIZE);
        return 0;
    }
    if (!memfd) {
        if (openFiles()) return 0;
    }
    if (!bcm_base) 
        bcm_base = bcm_host_get_peripheral_address();
    return (volatile uint32_t *)mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, memfd, bcm_base + offset);
}

void clearMemMap(void * map, uint32_t size) {
    munmap(map, size);
}

// Sends a pointer to the given buffer over to the VideoCore mailbox. See https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
void SendMailbox(void *buffer)
{
  int vcio = open("/dev/vcio", 0);
  if (vcio < 0) FATAL_ERROR("Failed to open VideoCore kernel mailbox!");
  int ret = ioctl(vcio, _IOWR(/*MAJOR_NUM=*/100, 0, char *), buffer);
  close(vcio);
  if (ret < 0) FATAL_ERROR("SendMailbox failed in ioctl!");
}

/* rounds size to nearest multiple of BCM_PAGESIZE and returns */
uint32_t ceilToPage(uint32_t size) {
    return (size & (PAGE_SIZE - 1)) ? size + PAGE_SIZE - (size & (PAGE_SIZE-1)): size;
}


// Sends a mailbox message with 1xuint32 payload
uint32_t Mailbox(uint32_t messageId, uint32_t payload0)
{
  MailboxMessage<1> msg(messageId);
  msg.payload[0] = payload0;
  SendMailbox(&msg);
  return msg.result;
}

// Sends a mailbox message with 3xuint32 payload
uint32_t Mailbox(uint32_t messageId, uint32_t payload0, uint32_t payload1, uint32_t payload2)
{
  MailboxMessage<3> msg(messageId);
  msg.payload[0] = payload0;
  msg.payload[1] = payload1;
  msg.payload[2] = payload2;
  SendMailbox(&msg);
  return msg.result;
}

#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

// Allocates the given number of bytes in GPU side memory, and returns the virtual address and physical bus address of the allocated memory block.
// The virtual address holds an uncached view to the allocated memory, so writes and reads to that memory address bypass the L1 and L2 caches. Use
// this kind of memory to pass data blocks over to the DMA controller to process.
GpuMemory AllocateUncachedGpuMemory(uint32_t numBytes)
{
  GpuMemory mem;
  mem.sizeBytes = ceilToPage(numBytes);
  mem.allocationHandle = Mailbox(MEM_ALLOC_MESSAGE, /*size=*/mem.sizeBytes, /*alignment=*/PAGE_SIZE, /*flags=*/MEM_ALLOC_FLAG_DIRECT);
  mem.busAddress = Mailbox(MEM_LOCK_MESSAGE, mem.allocationHandle);
  mem.virtualAddr = mmap(0, mem.sizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, BUS_TO_PHYS(mem.busAddress));
  if (mem.virtualAddr == MAP_FAILED) FATAL_ERROR("Failed to mmap GPU memory!");
  return mem;
}

void FreeUncachedGpuMemory(GpuMemory mem)
{
  munmap(mem.virtualAddr, mem.sizeBytes);
  Mailbox(MEM_UNLOCK_MESSAGE, mem.allocationHandle);
  Mailbox(MEM_FREE_MESSAGE, mem.allocationHandle);
}

#define DMA_TEST_CHANNEL 5

void dma_test_0() {
  printf("Mailbox approach:\n");
    
  void * virtSrcPage, *busSrcPage;
  GpuMemory srcPages = AllocateUncachedGpuMemory(PAGE_SIZE);
  virtSrcPage = srcPages.virtualAddr;
  busSrcPage = (void *)(srcPages.busAddress);

  DEBUG_PTR("virtSrc", virtSrcPage);
  DEBUG_PTR("physSrc", busSrcPage);

  void * virtDestPage, *busDestPage;
  GpuMemory destPages = AllocateUncachedGpuMemory(PAGE_SIZE);
  virtDestPage = destPages.virtualAddr;
  busDestPage = (void *)(destPages.busAddress);
  
  DEBUG_PTR("virtDest", virtDestPage);
  DEBUG_PTR("physDest", busDestPage);
    
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
	
  void * virtCBPage, *busCBPage;
  GpuMemory cbPages = AllocateUncachedGpuMemory(PAGE_SIZE);
  virtCBPage = cbPages.virtualAddr;
  busCBPage = (void*)(cbPages.busAddress);

  DEBUG_PTR("virtCB", virtCBPage);
  DEBUG_PTR("physCB", busCBPage);

  DMAControlBlock * cb = (DMAControlBlock *)virtCBPage;  

  cb -> transferInfo = SRC_INC | DEST_INC;
  cb -> srcAddr = (uint32_t)busSrcPage;
  cb -> destAddr = (uint32_t)busDestPage;
  cb -> transferLength = 13;
  cb -> stride = 0;
  cb -> reserved = 0;
  cb -> nextControlBlockAddr = 0;

  //enable DMA channel
	dmaMap[DMA_GLOBAL_ENABLE_REG] |= (1 << DMA_TEST_CHANNEL);

	dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL)] = DMA_RESET;
	sleep(1);

	// clear debug flags
	dmaMap[DMA_DEBUG_REG(DMA_TEST_CHANNEL)] = 7; 
	dmaMap[DMA_CONBLK_AD_REG(DMA_TEST_CHANNEL)] = (uint32_t)busCBPage; // temporarily...

  dmaMap[DMA_CS_REG(DMA_TEST_CHANNEL)] = 1;

  sleep(1);

  printf("destination reads: '%s'\n", (char*)virtDestPage);

  FreeUncachedGpuMemory(srcPages);
  FreeUncachedGpuMemory(destPages);
  FreeUncachedGpuMemory(cbPages);
}

void dma_test_1() {
  printf("Inconsequential test.\n");
  
    printf("Mailbox approach:\n");
    
  void * virtSrcPage, *busSrcPage;
  GpuMemory srcPages = AllocateUncachedGpuMemory(PAGE_SIZE);
  virtSrcPage = srcPages.virtualAddr;
  busSrcPage = (void *)(srcPages.busAddress);

  DEBUG_PTR("virtSrc", virtSrcPage);
  DEBUG_PTR("physSrc", busSrcPage);

  void * virtDestPage, *busDestPage;
  GpuMemory destPages = AllocateUncachedGpuMemory(PAGE_SIZE);
  virtDestPage = destPages.virtualAddr;
  busDestPage = (void *)(destPages.busAddress);
  
  DEBUG_PTR("virtDest", virtDestPage);
  DEBUG_PTR("physDest", busDestPage);
    
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
  
  FreeUncachedGpuMemory(srcPages);
  FreeUncachedGpuMemory(destPages);
}

int main() {
  openFiles();

  dmaMap = initMemMap(DMA_BASE_OFFSET, DMA_MAPSIZE);

  dma_test_0();
  dma_test_1();
  
  munmap((void *)dmaMap, DMA_MAPSIZE);
}

#include "pimem.h"

static uint32_t bcm_base = 0;
static uint32_t memfd = 0;
static uint32_t pagemapfd = 0;
static uint32_t vciofd = 0;

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

// used for calculations, not for an actual reference to bcm base, therefore returns an integer instead of a ptr
uint32_t getBCMBase() {
    if (!bcm_base)
        bcm_base = bcm_host_get_peripheral_address();
    return bcm_base;
}

/* rounds size to nearest multiple of BCM_PAGESIZE and returns */
uint32_t ceilToPage(uint32_t size) {
    return (size & (BCM_PAGESIZE - 1)) ? size + BCM_PAGESIZE - (size & (BCM_PAGESIZE-1)): size;
}

/* returns a map to memory of given size and @ address offset */
volatile uint32_t * initMemMap(uint32_t offset, uint32_t size) {
    if (offset % BCM_PAGESIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", BCM_PAGESIZE);
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

/* thanks to https://shanetully.com/2014/12/translating-virtual-addresses-to-physcial-addresses-in-user-space/
and https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example
for the following functions. I made some slight modifications to better fit my problem

initVirtPhysPage() and virtToPhys() are very similar. 

The former creates to pages, one with virtual memory, and one
with its corresponding phyiscal memory. 
*virtAddr and *physAddr point to the bases of these pages when initVirtPhysPage() returns

The latter returns a single void* == the phyiscal address correspoinding to virtAddr*

*/

/* 
returns a page of virtual memory that is guaranteed locked in place in RAM and won't be redirected to swap space
size should be a multiple of BCM_PAGESIZE --> if not, it will be rounded to the nearest BCM_PAGESIZE multiple 
*/
void * initLockedMem(uint32_t size) {
    size = ceilToPage(size);
    void * mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_NORESERVE|MAP_LOCKED, -1, 0);
    memset(mem, 0, size);
    mlock(mem, size);
    
    return mem;
}

void clearLockedMem(void * mem, uint32_t size) {
    munlock(mem, size);
    munmap(mem, size);    
}

/*
creates a page of virtual memory and updates *virtAddr to point to it. *physAddr updates to point to the corresponding physical address
of the virtual page
*/
void initVirtPhysPage(void ** virtAddr, void ** physAddr) {
    if (!pagemapfd) {
        if (openFiles()) return;
    }

    uint64_t pageInfo;
    
    // initialize virtual page, lock, clear
    *virtAddr = valloc(BCM_PAGESIZE);
    mlock(*virtAddr, BCM_PAGESIZE);
    memset(*virtAddr, 0, BCM_PAGESIZE);

    // (*virtAddr/BCM_PAGESIZE * PAGEMAP_LENGTH) == offset of pageInfo, so seek to that location in pagemapfd
    if ((lseek(pagemapfd, ((size_t)*virtAddr)/BCM_PAGESIZE * PAGEMAP_LENGTH, SEEK_SET)) < 0) {
        printf("Failure to seek page map to proper location.\n");
        return;
    }

    // we are concerned about bits 0 - 54 --> bit 55 is a flag
    read(pagemapfd, &pageInfo, PAGEMAP_LENGTH); //- 1);
    //pageInfo &= PAGE_INFO_MASK;
    
    // update *physAddr to point to the physical address corresponding to *virtAddr, given by (pageInfo*BCM_PAGESIZE)
    *physAddr = (void*)(size_t)(pageInfo*BCM_PAGESIZE);
    printf("Virtual page based at %p maps to physical address base %p\n", *virtAddr, *physAddr);
}

void clearVirtPhysPage(void * virtAddr) {
    munlock(virtAddr, BCM_PAGESIZE);
    free(virtAddr);
}

/*
returns a pointer to the corresponding physical address of *virtAddr
*/
uintptr_t virtToPhys(void * virtAddr) {
    if (!pagemapfd) {
        if (openFiles()) return 0;
    }
    
    // start w/ same procedure as initVirtPhysPage(). Seek to the region of pagemapfd that will give us the pageInfo
    
    uintptr_t pageNumber = ((uintptr_t) virtAddr) / BCM_PAGESIZE;
    uint32_t offset = (uint32_t) virtAddr % BCM_PAGESIZE;

    uint64_t pageInfo;
    int32_t err = lseek(pagemapfd, pageNumber*PAGEMAP_LENGTH, SEEK_SET);
    if (err != pageNumber*PAGEMAP_LENGTH) {
        printf("WARNING: failed to seek. Expected %i, got %i\n", pageNumber*PAGEMAP_LENGTH, err);
    }
    
    read(pagemapfd, &pageInfo, PAGEMAP_LENGTH); //- 1);
    
    if (DEBUG)
        printf("pageInfo = %llu\n", pageInfo);//DEBUG_VAL("page info", pageInfo);

    if (!(pageInfo & (1ull<<63))) {//0x8000000000000000)) {
        printf("WARNING: %p has no phyiscal address!\n", virtAddr);
    }

    // we are concerned about bits 0 - 54 --> bit 55 is a flag
    pageInfo &= PAGE_INFO_MASK;
    return (uintptr_t)(pageInfo*BCM_PAGESIZE + offset);
}

/*
returns a corresponding physical address ptr for virtAddr that bypasses L1 cache.
if useDirectUncached == 0, will return an address that also bypasses L2 (direct uncached memory)
if useDirectUncached == 1, will return an address that is L2 cache-coherent
*/
uintptr_t virtToUncachedBus(void * virtAddr, bool useDirectUncached) {
    return useDirectUncached ? (virtToPhys(virtAddr) | DIRECT_UNCACHED_BASE) : (virtToPhys(virtAddr) | L2_COHERENT_BASE);
}

/* mailbox interface funcs

sources: 
https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
https://github.com/raspberrypi/userland/blob/master/host_applications/linux/apps/vcmailbox/vcmailbox.c

 */

// WARNING: mixed information on if L2 coherent base is at 0x4... or 0x8...
uintptr_t busToPhys(void * busAddr, bool useDirectUncached) {
    return useDirectUncached ? ((uintptr_t)busAddr & ~DIRECT_UNCACHED_BASE) : ((uintptr_t)busAddr & ~L2_COHERENT_BASE);
}

static void mailboxWrite(void * message) {
    if (!vciofd)
        openFiles();
    int ret;
    if ((ret = ioctl(vciofd, _IOWR(100, 0, char *), message)) < 0)
        printf("ERROR: Failed to send mailbox data. Error no: %d\n", errno);
}

static uint32_t sendMailboxMessage(uint32_t messageId, uint32_t payload) {
    uint32_t message[7];
    
    message[0] = 28; // size in bytes
    message[1] = 0; // request code
    message[2] = messageId;
    message[3] = 4; // payload size in bytes
    message[4] = 4;
    message[5] = payload;
    message[6] = 0; // message end
    
    mailboxWrite(&message);
    return message[5]; // result
}

static uint32_t sendMailboxMessages(uint32_t messageId, uint32_t payload[], uint32_t payloadSize) {
    uint32_t payloadSizeInBytes = payloadSize << 2;    
    uint32_t message[7];
    
    message[0] = 24 + payloadSizeInBytes;
    message[1] = 0;
    message[2] = messageId;
    message[3] = payloadSizeInBytes;
    message[4] = payloadSizeInBytes;
    for (uint32_t i = 0; i < payloadSize; i++) {
        message[5 + i] = payload[i];
    }
    message[5 + payloadSize] = 0;
    
    mailboxWrite(&message);
    return message[5];
}

// TODO: zero out returned mem?
VirtToBusPages initUncachedMemView(uint32_t size, bool useDirectUncached) {
    VirtToBusPages mem;
    mem.size = ceilToPage(size);

    uint32_t cacheFlags = useDirectUncached ? MAILBOX_MEM_FLAG_DIRECT : MAILBOX_MEM_FLAG_COHERENT;

    uint32_t mallocPayload[3] = {
        mem.size,
        BCM_PAGESIZE,
        cacheFlags
    };

    mem.allocationHandle = sendMailboxMessages(MAILBOX_MALLOC_TAG, mallocPayload, 3);
    mem.busAddr = sendMailboxMessage(MAILBOX_MLOCK_TAG, mem.allocationHandle);
    mem.virtAddr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, memfd, (uint32_t)busToPhys((void *)mem.busAddr, useDirectUncached));

    return mem;
}

void clearUncachedMemView(VirtToBusPages * mem) {
    munmap(mem->virtAddr, mem->size);
    sendMailboxMessage(MAILBOX_MUNLOCK_TAG, mem->allocationHandle);
    sendMailboxMessage(MAILBOX_FREE_TAG, mem->allocationHandle);
}

#include "pimem.h"

static uint32_t bcm_base = 0;
static uint32_t memfd = 0;
static uint32_t pagemapfd = 0;

// initializes memfd and pagemapfd, which are used by multiple functions in this file
static bool openFiles() {
    if (!memfd) {
        if((memfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
            printf("Failure to access /dev/mem\n"); 
            return 1;
        }
    }
    if (!pagemapfd) {
        if((pagemapfd = open("/proc/self/pagemap")) < 0) {
            printf("Failure to access /proc/self/pagemap.\n");
            return 1;
        }
    }
    return 0;
}

// used for calculations, not for an actual reference to bcm base, therefore returns an integer instead of a ptr
uint32_t getBCMBase() {
    return bcm_base;
}

static uint32_t ceilToPage(uint32_t size) {
    return (size & (BCM_PAGESIZE - 1)) ? size + BCM_PAGESIZE - (size & (BCM_PAGESIZE-1)): size;
}

void * initMemMap(uint32_t offset, uint32_t size) {
    if (offset % BCM_PAGESIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", BCM_PAGESIZE);
        return 0;
    }
    if (!memfd) {
        if (openFiles()) return 0;
    }    
    return mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, memfd, bcm_base + offset);
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

initUncachedMemView() solves the problem of virtual addresses passing thru the Pi's L1 cache when mapping

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

void initVirtPhysPage(void ** virtAddr, void ** physAddr) {
    if (!pagemapfd) {
        if (openFiles()) return;
    }

    uint64_t pageInfo;
    *virtAddr = valloc(BCM_PAGESIZE);
    mlock(*virtAddr, BCM_PAGESIZE);
    memset(*virtAddr, 0, BCM_PAGESIZE);

    if ((lseek(pagemapfd, (uint64_t)*virtAddr/BCM_PAGESIZE * PAGEMAP_LENGTH, SEEK_SET)) < 0) {
        printf("Failure to seek page map to proper location.\n");
        return 0;
    }

    read(pagemapfd, &pageInfo, PAGEMAP_LENGTH - 1);
    pageInfo &= PAGE_INFO_MASK;
    
    *physAddr = (void*)(pageInfo*BCM_PAGESIZE);
    printf("Virtual page based at %p maps to physical address base %p\n", *virtAddr, *physAddr);
}


void * virtToPhys(void * virtAddr) {
    if (!pagemapfd) {
        if (openFiles()) return 0;
    }

    uint32_t * pageNumber = (uint32_t *) virtAddr / BCM_PAGESIZE;
    uint32_t offset = (uint32_t) virtAddr % BCM_PAGESIZE;

    uint64_t pageInfo;
    int32_t err = lseek(pagemapfd, pageNumber*PAGEMAP_LENGTH, SEEK_SET);
    if (err != pageNumber*PAGEMAP_LENGTH) {
        printf("WARNING: failed to seek. Expected %i, got %i\n", pageNumber*PAGEMAP_LENGTH, err);
    }
    
    read(pagemapfd, &pageInfo, PAGEMAP_LENGTH - 1);

    if (!(pageInfo & 0x8000000000000000)) {
        printf("WARNING: %p has no phyiscal address!\n", virtAddr);
    }

    pageInfo &= PAGE_INFO_MASK;
    return (void *)(pageNumber*BCM_PAGESIZE + offset);
}

void * virtToUncachedPhys(void * virtAddr, bool useDirectUncached) {
    return useDirectUncached ? (virtAddr | DIRECT_UNCACHED_BASE) : (virtAddr | L2_COHERENT_BASE);
}

void * initUncachedMemView(void * virtAddr, uint32_t size) {
    size = ceilToPage(size);

    // allocate arbitrary virtual mem then immediately free it
    void * mem = mmap(NULL, size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_LOCKED|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
    munmap(mem, size);

    // iterate page by page of uncached mem space, verifying that mem is contiguous
    for (int offset = 0; offset < size; offset += BCM_PAGESIZE) {
        void * mappedPage = mmap(mem + offset, BCM_PAGESIZE, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FIXED|MAP_NORESERVE, memfd, virtToPhys(virtAddr + offset));
        if (mappedPage != mem + offset) {
            printf("Failed to map contiguous uncached memory.\n");
            return 0;
        }
    }

    memset(mem, 0, size);
    return mem;
}

void clearUncachedMemView(void * mem, uint32_t size) {
    size = ceilToPage(size);
    munmap(mem, size);
}

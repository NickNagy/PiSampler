#include "pimem.h"

static unsigned bcm_base = 0;

// used for calculations, not for an actual reference to bcm base, therefore returns an integer instead of a ptr
unsigned getBCMBase() {
    return bcm_base;
}

unsigned ceilToPage(unsigned size) {
    return (size & (BCM_PAGESIZE - 1)) ? size + BCM_PAGESIZE - (size & (BCM_PAGESIZE-1)): size;
}

void * initMemMap(unsigned offset, unsigned size) {
    int fd;
    if (offset % BCM_PAGESIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", BCM_PAGESIZE);
        return 0;
    }
    size = ceilToPage(size);
    if (!bcm_base) {
        if ((fd = open("/dev/mem", O_RDWR | O_SYNC) < 0) {
            printf("Failure to access /dev/mem\n"); 
            return 0;
        }
        if (DEBUG) printf("fd initialized.\n");
        bcm_base = bcm_host_get_peripheral_address();
    }
    return mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, fd, bcm_base + offset);
}

void clearMemMap(void * map, unsigned size) {
    munmap(map, size);
}

void * toUncachedAddr(void * virtAddr, bool useDirectUncached) {
    return useDirectUncached ? (virtAddr | DIRECT_UNCACHED_BASE) : (virtAddr | L2_COHERENT_BASE);
}

void * initLockedMem(unsigned size) {
    int fd;
    size = ceilToPage(size);
    void * mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_NONRESERVE|MAP_LOCKED, -1, 0);
    memset(mem, 0, size);
    mlock(mem, size);
    
    return mem;
}

void clearLockedMem(void * mem, unsigned size) {
    munlock(mem, size);
    munmap(mem, size);    
}

// thanks to https://shanetully.com/2014/12/translating-virtual-addresses-to-physcial-addresses-in-user-space/
// and https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example
void initVirtPhysPage(void ** virtAddr, void ** physAddr) {
    int fd;
    unsigned long pageInfo;
    *virtAddr = valloc(BCM_PAGESIZE);
    mlock(*virtAddr, BCM_PAGESIZE);
    memset(*virtAddr, 0, BCM_PAGESIZE);
    
    if ((fd = open("proc/self/pagemap", O_RDWR)) < 0) {
        printf("Failure to access proc/self/pagemap.\n");
        return 0;    
    }        
    if (lseek(fd, (unsigned long)*virtAddr/BCM_PAGESIZE * PAGEMAP_LENGTH, SEEK_SET)) {
        printf("Failure to seek page map to proper location.\n");
        return 0;
    }

    read(fd, &pageInfo, PAGEMAP_LENGTH - 1);
    pageInfo &= 0x7FFFFFFFFFFFFFFF; // bits 0 - 54
    
    *physAddr = (void*)(pageInfo*BCM_PAGESIZE);
    printf("Virtual page based at %p maps to physical address base %p\n", *virtAddr, *physAddr);
}

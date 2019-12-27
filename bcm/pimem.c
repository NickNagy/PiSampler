#include "pimem.h"

static unsigned bcm_base = 0;
static int fd = 0;

unsigned * initMemMap(unsigned offset, unsigned size) {
    if (offset % BCM_PAGESIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", BCM_PAGESIZE);
        return 0;
    }
    if (!bcm_base) {
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd < 0) {
            printf("Failure to access /dev/mem\n"); 
            return 0;
        }
        if (DEBUG) printf("fd initialized.\n");
        bcm_base = bcm_host_get_peripheral_address();
    }
    return (unsigned *)mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, bcm_base + offset);
}

void clearMemMap(unsigned * map, unsigned size) {
    munmap((void*)map, size);
}

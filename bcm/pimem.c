#include "pimem.h"

static unsigned bcm_base = 0;
static int fd = 0;

unsigned * initMemMap(unsigned offset, unsigned size) {
    if (!bcm_base) {
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd < 0) {
            printf("Failure to access /dev/mem\n"); 
            return 0;
        }
        bcm_base = bcm_host_get_peripheral_address();
    }
    return (unsigned *)mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, bcm_base + offset);
}

void clearMemMap(unsigned * map, unsigned size) {
    munmap((void*)map, size);
}
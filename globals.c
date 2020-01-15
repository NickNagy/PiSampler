#include "globals.h"

void * getAlignedPointer(void * ptr, int32_t byteAlignment) {
    if (!ptr) {
        ERROR_MSG("Uninitialized pointer. Aborting without update.");
        exit(1);
    } else if (byteAlignment % 2 != 0) { // TODO: this logic is incorrect!!
        ERROR_MSG("Byte alignment must be a power of 2. Pointer not updated.");
        exit(1);
    } else {
        int32_t ptrRelevantBits = (int32_t)ptr & ~(~byteAlignment + 1);
        if (!ptrRelevantBits) // already aligned
                return ptr;
        return (void*)((char *)ptr + (byteAlignment - ptrRelevantBits));
    }
    return ptr;
}

// from pigpio.c --> https://github.com/joan2937/pigpio/blob/master/pigpio.c
unsigned gpioDelay(uint32_t micros) {
    return 0;
}

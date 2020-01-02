#include "globals.h"

void * getAlignedPointer(void * ptr, int byteAlignment) {
    if (!ptr) {
        printf("ERROR! Uninitialized pointer. Aborting without update.\n");
    } else if (byteAlignment % 2 != 0) { // TODO: this logic is incorrect!!
        printf("ERROR! Byte alignment must be a power of 2. Pointer not updated.\n");
    } else {
        int ptrRelevantBits = (int)ptr & ~(~byteAlignment + 1);
        if (!ptrRelevantBits) // already aligned
                return ptr;
        return (void*)((char *)ptr + (byteAlignment - ptrRelevantBits));
    }
    return ptr;
}

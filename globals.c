#include "globals.h"

void * getAlignedPointer(void * ptr, int byteAlignment) {
    if (byteAlignment % 2 != 0) {
        printf("ERROR! Byte alignment must be a power of 2. Pointer not updated.\n");
    } else {
        if (DEBUG) printf("pointer before = %p\n", ptr);
        int twosCompByteAlignment = ~byteAlignment + 1;
        int ptrRelevantBits = (int)ptr & ~twosCompByteAlignment;
        int bytesAway = byteAlignment - ptrRelevantBits;
        if (DEBUG) printf("2's comp alignment = %x\n", twosCompByteAlignment);
        DEBUG_VAL("relevant ptr bits", ptrRelevantBits);
        DEBUG_VAL("bytes from next aligned address", bytesAway);
        ptr = (void*)((char *)ptr + bytesAway);
        if (DEBUG) printf("pointer after = %p\n", ptr);
    }
    return ptr;
}

#include "globals.h"

void * getAlignedPointer(void * ptr, int byteAlignment) {
    if (byteAlignment % 2 != 0) {
        printf("ERROR! Byte alignment must be a power of 2. Pointer not updated.\n");
    } else {
        if (DEBUG) printf("pointer before = %x\n", ptr);
        int twosCompByteAlignment = ~byteAlignment + 1;
        int ptrRelevantBits = ptr & ~twosCompByteAlignment;
        int bytesAway = byteAlignment - ptrRelevantBits;
        if (DEBUG) printf("2's comp alignment = %x\n", twosCompByteAlignment);
        DEBUG_VAL("relevant ptr bits", ptrRelevantBits);
        DEBUG_VAL("bytes from next aligned address", bytesAway);
        ptr += bytesAway;
        if (DEBUG) printf("pointer after = %x\n", ptr);
    }
    return ptr;
}
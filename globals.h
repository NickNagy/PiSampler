#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

// for debugging purposes
#define DEBUG 1
#define VERBOSE 1

#define DEBUG_MSG(msg) if (DEBUG) printf(msg);
#define VERBOSE_MSG(msg) if (VERBOSE) printf(msg);

#define PRINT_REG(name, reg) printf("%s at address %p = %x\n", name, &reg, reg);
#define COND_PRINT_REG(cond, name, reg) if (cond) { PRINT_REG(name, reg) }
#define DEBUG_REG(name, reg) COND_PRINT_REG(DEBUG, name, reg)

#define PRINT_VAL(name, val) printf("%s = %d\n", name, val);
#define COND_PRINT_VAL(cond, name, val) if (cond) PRINT_VAL(name, val);
#define DEBUG_VAL(name, val) COND_PRINT_VAL(DEBUG, name, val)

#define WARN_MSG(msg)  printf("WARNING: %s\n", msg); 
#define ERROR_MSG(msg) printf("ERROR: %s\n", msg); 

void * getAlignedPointer(void * ptr, int32_t byteAlignment);

// from pigpio.c --> https://github.com/joan2937/pigpio/blob/master/pigpio.c
unsigned gpioDelay(uint32_t micros);

#endif

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

#define DEBUG_MSG(msg) if (DEBUG) printf("%s\n",msg);
#define VERBOSE_MSG(msg) if (VERBOSE) printf("%s\n",msg);

#define PRINT_REG(name, reg) printf("%s at address %p = %x\n", name, &reg, reg);
#define COND_PRINT_REG(cond, name, reg) if (cond) { PRINT_REG(name, reg) }
#define DEBUG_REG(name, reg) COND_PRINT_REG(DEBUG, name, reg)

#define PRINT_VAL(name, val) printf("%s = %d\n", name, val);
#define COND_PRINT_VAL(cond, name, val) if (cond) PRINT_VAL(name, val);
#define DEBUG_VAL(name, val) COND_PRINT_VAL(DEBUG, name, val)

#define PRINT_PTR(name, ptr) printf("%s = %p\n", name, ptr);
#define COND_PRINT_PTR(cond, name, ptr) if (cond) PRINT_PTR(name, ptr);
#define DEBUG_PTR(name, ptr) COND_PRINT_PTR(DEBUG, name, ptr);

#define WARN_MSG(msg)  printf("WARNING: %s\n", msg); 
#define ERROR_MSG(msg) printf("ERROR: %s\n", msg); 
#define FATAL_ERROR(msg) { ERROR_MSG(msg); exit(1); }

#endif

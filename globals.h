#ifndef GLOBALS_H
#define GLOBALS_H

// for debugging purposes
#define DEBUG 1
#define VERBOSE 0

#define PRINT_REG(name, reg) printf("%s at address %p = %x\n", name, &reg, reg);
#define COND_PRINT_REG(cond, name, reg) if (cond) { PRINT_REG(name, reg) }
#define DEBUG_REG(name, reg) COND_PRINT_REG(DEBUG, name, reg)

#define PRINT_VAL(name, val) printf("%s = %d\n", name, val);
#define COND_PRINT_VAL(cond, name, val) if (cond) PRINT_VAL(name, val);
#define DEBUG_VAL(name, val) COND_PRINT_VAL(DEBUG, name, val)

#endif

#ifndef INTERRUPT
#define INTERRUPT

#include "../h/types.h"

extern void interruptHandler();
extern void devIntHelper(int tempnum);
extern void copyState(state_PTR first, state_PTR copy);

#endif

#ifndef INTERRUPT
#define INTERRUPT

extern void interruptHandler();
extern int getDeviceNumber(unsigned int* bitMap);
extern void localTimer();
extern void copyState(state_PTR first, state_PTR copy);

#endif

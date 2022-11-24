#ifndef INTERRUPT
#define INTERRUPT

extern void interruptHandler();
extern int getDeviceNumber(unsigned int* bitMap);
extern void localTimer(cpu_t startTime);
extern void copyState(state_PTR first, state_PTR copy);

#endif

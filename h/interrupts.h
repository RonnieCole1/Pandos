#ifndef INTERRUPT
#define INTERRUPT

extern void interruptHandler();
extern void devIntHelper(int tempnum);
extern void copyState(state_PTR first, state_PTR copy);
extern void readyTimer(pcb_PTR cp, cpu_t time);

#endif

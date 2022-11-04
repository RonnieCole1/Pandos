#ifndef SCHED
#define SCHED

/************************* SCHEDULER.H *****************************
*
*  The externals declaration file for the Scheduler.
*
*  Authors:
*       Joseph Counts
*       Ronnie Cole
*       Joe Pinkerton
*/

extern void scheduler();
extern void Load_State(state_PTR currentProcess);
extern void contSwitch(pcb_t *currProc);
extern void finalMSG(char msg[], bool Bstatus);
extern void Move_Process(pcb_PTR p);
extern void uTLB_RefillHandler();

#endif

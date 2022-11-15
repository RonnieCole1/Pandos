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

#include "../h/types.h"

extern void scheduler();
extern void context_Switch(pcb_t *currProc);

#endif

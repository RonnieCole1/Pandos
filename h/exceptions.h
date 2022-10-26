#ifndef EXCEPT
#define EXCEPT

/************************* EXCEPTIONS.H *****************************
*
*  The externals declaration file for the Exception Handling.
*
*  Authors:
*       Joseph Counts
*       Ronnie Cole
*       Joe Pinkerton
*/

#include "../h/types.h"

extern void SYSCALL();
extern void Create_ProcessP();
extern void Terminate_Process();
extern pcb_t *wait(sema4);
extern pcb_t *signal(sema4);
extern void Wait_for_IO_Device();
extern int Get_CPU_Time(pcb_t *p);
extern void Wait_For_Clock();
extern void BlockedSYS(pcb_t *p);

#endif

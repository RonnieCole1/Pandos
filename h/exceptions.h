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

extern void systemCall();
extern void Create_ProcessP(state_t *caller);
extern void Terminate_Process();
extern void wait(state_PTR caller);
extern void signal(state_PTR caller);
extern void Wait_for_IO_Device(state_PTR caller);
extern int Get_CPU_Time(state_PTR caller);
extern void Wait_For_Clock(state_PTR caller);
extern void Get_SUPPORT_Data();
extern void programTRPHNDLR();
extern void TLB_TrapHandler();
extern void passUpOrDie(int ExeptInt);



#endif

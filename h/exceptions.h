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
extern void Create_ProcessP();
extern void Terminate_Process();
extern pcb_t *wait();
extern pcb_t *signal();
extern void Wait_for_IO_Device();
extern int Get_CPU_Time();
extern void Wait_For_Clock();
extern void BlockedSYS();

#endif

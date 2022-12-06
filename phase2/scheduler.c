#include "../h/types.h"
#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Scheduler ****************************
 *
 *   
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

/* ---------------- External Global Variables ---------------- */
extern cpu_t TODStarted;
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;

void scheduler() {
    cpu_t currentTOD;
    /* If our current process is not null, find our accumulated time and load
    our timer onto MILLI. */
    if(currentProc != NULL){
        STCK(currentTOD);
        currentProc->p_time = (currentProc->p_time) + (currentTOD - TODStarted);
        LDIT(MILLI);
    }
    
    /* Dispatch the "next" process in the Ready Queue */
    if(!emptyProcQ(readyQue)){      
        currentProc = removeProcQ(&readyQue);
        /* Get the start time */
        STCK(TODStarted);
        /* Load 5ms on PLT */
        setTIMER(TIMESLICE);   
        /* Load processor state */
        myLDST(&(currentProc->p_s));    
    } else {
        /* If our process count is 0, we Halt our program. Our job is finished */
        if(processCnt == 0){
            HALT();
        } else {
            /* Otherwise, if SBC is not 0, set a new value in our status register
            and wait. */
            if(softBlockCnt != 0){
                /* wait */
                setSTATUS(ALLOFF | IEMON | IMON);
                WAIT();
            /* If we've reached this point, we've encountered a deadlock.
            Panic! */
            } else {
                PANIC();
            }
        }
    }
}

/* Created for the sake of debugging purposes. Atomically load our processor state with state s*/
void myLDST(state_PTR s) {
	LDST(s);
}

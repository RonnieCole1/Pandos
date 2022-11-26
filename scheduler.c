#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/initial.h"
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

/* global variables maintaining time usage*/
extern cpu_t TODStarted;
cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;

void scheduler() {
    if(currentProc != NULL){
        STCK(currentTOD);
        currentProc->p_time = (currentProc->p_time) + (currentTOD - TODStarted);
    }

    /* Dispatch the "next" process in the Ready Queue */
    if(!emptyProcQ(readyQue)){      
        currentProc = removeProcQ(&readyQue);
        STCK(TODStarted);           /* Get the start time */
        setTIMER(TIMESLICE);        /* Load 5ms on PLT */
        LDST(&(currentProc->p_s));    /* Load processor state */
    } else{
        currentProc = NULL;

        /* Job well done*/
        if(processCnt == 0){
            HALT();
        } else{
            if(softBlockCnt != 0){
                /* wait */
                setSTATUS(ALLOFF | IEMON | IMON);
                WAIT();
            } else{
                /* deadlock */
                PANIC();
            }
        }
    }
}

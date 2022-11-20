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
extern cpu_t TODStarted; /* Once initilized in scheduler(), it keeps track of the time since process started. */
extern cpu_t currentTOD; /* Used relative to TODStarted to find CPU time */

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;


void scheduler() {
	/*Process is pulled off of the ready Que*/
	pcb_t *p = removeProcQ(&readyQue);
	/*If this new process is not NULL...*/
    if(p != NULL){
    	currentProc = p;
    	/*Start our TODStarted Clock.*/
        STCK(TODStarted);
        /*Set our timer to our timeslice (quantom)*/
        setTIMER(TIMESLICE);
        myLDST(&(currentProc->p_s));
    }
    if (processCnt == 0) {
        HALT();
    }
    if (processCnt != 0) {
    	if(softBlockCnt != 0){
        	/* wait */
                setTIMER(LARGETIMEVALUE);
                setSTATUS(ALLOFF | IECON | IMON);
                WAIT();
            }
         if(softBlockCnt ==0) {
                /* deadlock */
                PANIC();
            }
    }
}

/* Created for the sake of debugging purposes. Atomically load our processor state with state s*/
void myLDST(state_PTR s) {
	LDST(s);
}



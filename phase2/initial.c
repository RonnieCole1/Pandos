#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "p2test.c"

/************************************ Nucleus Initialization ****************************
 *
 *   
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

extern void test();
extern void uTLB_RefillHandler();

/* 
    Declare global variables 
*/
int procssCnt;          /* int indicating the number of strated, but not yet terminated processes */
int softBlockCnt;       /* number of started, but not terminated processes that are in the "blocked" statevdue to an I/O or timer request*/
pcb_t *readyQue;        /* tail pointer to a queue of pcbs that are in the "ready" state */
pcb_t *currentProc;     /* pointer to the pcb that is in the "running" state */
int deviceSema4s[MAXDEVICECNT]; 

/* 
    Programs entry point performing the Nucleus initialization
*/
int main(){
    /* Load system-wide interval timer */
    int RAMTOP;
    devregarea_t *top;
    top = (devregarea_t *) RAMBASEADDR;
    RAMTOP = top->rambase + top->ramsize;
    top->intervaltimer = 100;               /* 100 milliseconds */

    /* Populate the Processor 0 Pass Up Vector */
    passupvector_t *temp;
    temp = (passupvector_t *) PASSUPVECTOR;
    temp->tlb_refill_handler = (memaddr) uTLB_RefillHandler();
    temp->tlb_refill_stackPtr = KERNELSTACK;
    temp->exception_handler = (memaddr) genExceptionHandler();
    temp->exception_stackPtr = KERNELSTACK;

    /* Initialize PCB and ASL data structures */
    initPcbs();
    initASL();

    /* Initialize all Nucleus maintained variables */
    procssCnt = softBlockCnt = 0;
    readyQue = mkEmptyProcQ();
    currentProc = NULL;

    int i;
    for(i = 0; i < MAXDEVICECNT; i++){
        deviceSema4s[i] = 0;
    }

    /* Instantiate a single process */
    currentProc = allocPcb();
    if(currentProc != NULL){
        currentProc->p_s.s_sp = RAMTOP;
        currentProc->p_s.s_pc = (memaddr) test; /* test function in p2test */
        currentProc->p_s.s_t9 = (memaddr) test;
        currentProc->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
        insertProcQ(&readyQue, currentProc);
        procssCnt += 1;
        currentProc = NULL;
    } else{
        PANIC();
    }

    LDIT(INTERVALTMR);

    /* Call the Scheduler */
    scheduler();
}

/*

*/
void genExceptionHandler(){
    state_PTR oldState = (state_t *) BIOSDATAPAGE;
    int temp;
    temp = (oldState->s_cause & GETEXECCODE) >> CAUSESHIFT;

    if(Cause.ExcCode == INTERUPTHANDLER)
    {
        /*Pass along to interput handler (not yet implemented)*/
    }
    /*TLB Exceptions*/
    if(Cause.ExcCode <= 3 && Cause.ExcCode >= 1)
    {
        uTLB_RefillHandler();
    }
    /*Program Traps*/
    if((Cause.ExcCode <= 4 && Cause.ExcCode >= 7 )
    && (Cause.ExcCode <= 9 && Cause.ExcCode >= 12 ))
    {
        /*Pass along to program trap handler*/
    }
    /*SYSCALL*/
    if(Cause.ExcCode == 8)
    {
        SYSCALL(/*SYSNUM*/);
    }
}

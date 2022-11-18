#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

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
extern void genExceptionHandler();

/* 
    Declare global variables 
*/
int processCnt;          /* int indicating the number of strated, but not yet terminated processes */
int softBlockCnt;       /* number of started, but not terminated processes that are in the "blocked" statevdue to an I/O or timer request*/
pcb_t *readyQue;        /* tail pointer to a queue of pcbs that are in the "ready" state */
pcb_t *currentProc;     /* pointer to the pcb that is in the "running" state */
int deviceSema4s[MAXDEVICECNT];
cpu_t TODStarted;

/* 
    Programs entry point performing the Nucleus initialization
*/
int main(){

    /* Define RAMTOP, and have it refer to the last frame */
    memaddr RAMTOP;
    /*RAMTOP = RAMBASESIZE + RAMBASEADDR;*/

    /* set interval timer to 100 milliseconds */
    devregarea_t *top;
    top = (devregarea_t *) RAMBASEADDR;
    top->intervaltimer = 100;
    RAMTOP = top->rambase + top->ramsize;

    /* Populate the Processor 0 Pass Up Vector */
    passupvector_t *pvector;
    pvector = (passupvector_t *) PASSUPVECTOR;
    pvector->tlb_refll_handler = (memaddr) uTLB_RefillHandler;
    pvector->tlb_refll_stackPtr = KERNALSTACK;
    pvector->execption_handler = (memaddr) genExceptionHandler;
    pvector->exception_stackPtr = KERNALSTACK;

    /* Initialize PCB and ASL data structures */
    initPcbs();
    initASL();

    /* Initialize all Nucleus maintained variables */
    processCnt = 0;
    softBlockCnt = 0;
    readyQue = mkEmptyProcQ();
    currentProc = NULL;

    /* Initialize the semaphores. There are 49 semaphores, including our clock. */
    int i;
    for(i = 0; i < MAXDEVICECNT; i++){
        /* These semaphores start as synchronization semaphores, including our clock semaphore @49*/
        deviceSema4s[i] = 0;
    }

    /* Instantiate a single process */
    currentProc = allocPcb();
    /* If our current proc is not Null, then initalize all of currentProc's fields*/
    if(currentProc != NULL){
        /*Set currentProc's state's stack pointer to our interval timer*/
        currentProc->p_s.s_sp = (memaddr) RAMTOP;
        currentProc->p_s.s_pc = currentProc->p_s.s_t9 = (memaddr) test; /* test function in p2test */
        currentProc->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
        currentProc->p_supportStruct = NULL;

        /*Insert CurrentProc into our readyQue*/
        insertProcQ(&readyQue, currentProc);
        processCnt += 1;
        /*currentProc = NULL; not sure we need this but maybe we do*/     
        /* Call the Scheduler */
        scheduler();
    } else{
        PANIC();
    }
    LDIT(PCLOCKTIME);
}

/*

*/
void genExceptionHandler(){
    state_PTR oldState = (state_t *) BIOSDATAPAGE;
    int exeCause;
    exeCause = (oldState->s_cause & GETEXECCODE) >> CAUSESHIFT;

    if(exeCause == INTERRUPTHANDLER){
        interruptHandler();         /* Interrupt Handler */
    } else if(exeCause <= TLBEXCEPTS){
        TLB_TrapHandler();          /* TLB Exceptions */
    } else if(exeCause == SYSCALLEXECPTS){
        systemCall();               /* SYSCALL */
    } else{
        programTRPHNDLR();
    }
}
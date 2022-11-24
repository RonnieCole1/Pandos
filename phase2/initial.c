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
extern void scheduler();

int processCnt; /* Integer indicating the number of started, but not yet terminated processes */

int softBlockCnt; /* Number of started, but not terminated processes that are in the "blocked" statevdue to an I/O or timer request*/

pcb_t *readyQue; /* tail pointer to a queue of pcbs that are in the "ready" state */

pcb_t *currentProc; /* pointer to the pcb that is in the "running" state */

int deviceSema4s[MAXDEVICECNT]; /* Integer array of device semaphores including our Pseudoclock semaphore */

cpu_t TODStarted; /* Global variable used to keep track of the time since our process started */

cpu_t currentTOD; /* Used relative to TODStarted to find CPU time */


/* 
    Programs entry point performing the Nucleus initialization
*/
int main(){

    /* Define RAMTOP, and have it refer to the last frame */
    devregarea_t* dBus = (devregarea_t*) RAMBASEADDR;
    int RAMTOP = (dBus->rambase + dBus->ramsize);


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
    
     /* Write time onto the pseudoClock semaphore*/
    LDIT(PCLOCKTIME);
    
    pcb_PTR p = allocPcb();
    if(p != NULL){
    
        /*Set currentProc's state's stack pointer to our interval timer*/
        p->p_s.s_sp = (memaddr) RAMTOP;
        p->p_s.s_pc = p->p_s.s_t9 = (memaddr) test; /* test function in p2test */
        p->p_s.s_status = (ALLOFF | IECON | IMON | TEBITON);
        p->p_supportStruct = NULL;
        /*Insert CurrentProc into our readyQue*/
        insertProcQ(&readyQue, p);
        processCnt ++;
         
        /* Call the Scheduler */
        scheduler();
    }
    return 0;
}

/*
	General Exception Handler. Depending on the value obtained from the cause register, we pick 4
	types of exception handling. 
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

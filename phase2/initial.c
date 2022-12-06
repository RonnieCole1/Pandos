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
 *   The Nucleus contains two functions: An entry point declared main() which preforms
 *   the nucleus initialization and a geneeral exception handler that directs us to
 *   the proper exception handler. Asside from initilizing our global variables, we also
 *   populate the Pass Up Vector and instantiate a single process. At the end of main(),
 *   we call scheduler.
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

extern void test();
extern void uTLB_RefillHandler();
extern void genExceptionHandler();

/* --------------------- Global Variables --------------------- */

/* int indicating the number of strated, but not yet terminated processes */
int processCnt;

/* number of started, but not terminated processes that are in the "blocked" 
state due to an I/O or timer request*/
int softBlockCnt;

/* tail pointer to a queue of pcbs that are in the "ready" state */
pcb_t *readyQue; 
 
/* pointer to the pcb that is in the "running" state */    
pcb_t *currentProc;

/* Integer array of device semaphores including our clock semaphore */
int deviceSema4s[MAXDEVICECNT];

/* Address of our clock semaphore */
int *ClockSema4 = &deviceSema4s[MAXDEVICECNT-1];

/* Global variable used to keep track of the time since our process started */
cpu_t TODStarted;

/************************************************************************
Main() serves as the entry point for Pandos. Global variables are initlized.
Pass Up vectors are created so that the BIOS can find the address of our
Nucleus functions so that we can pass control for TLB-Refill events and other
exceptions. Phase 1 data structures are initilized. Our system-wide Interval
Timer is loaded with 100 milliseconds. The name for our interval timer is
MILLI. We instantiate a single process p and place it in our ready queue.
************************************************************************/
int main(){

    /* ------------------- Local Variables ------------------- */
    memaddr RAMTOP;
    devregarea_t *top;
    passupvector_t *pvector;
    
    /* --------- Initilize Ramtop and interval timer  --------- */
    top = (devregarea_t *) RAMBASEADDR;
    
    /* set interval timer to 100 milliseconds */
    top->intervaltimer = 100;
    
    /* Have RAMTOP refer to the last RAM frame */
    RAMTOP = top->rambase + top->ramsize;
    
    /* Have RAMTOP refer to the last RAM frame */
    pvector = (passupvector_t *) PASSUPVECTOR;
    
    /* ----------- Populate Processor 0 Passup Vector ----------- */
    pvector = (passupvector_t *) PASSUPVECTOR;
    pvector->tlb_refll_handler = (memaddr) uTLB_RefillHandler;
    pvector->tlb_refll_stackPtr = KERNALSTACK;
    pvector->execption_handler = (memaddr) genExceptionHandler;
    pvector->exception_stackPtr = KERNALSTACK;
    /* ----------------------------------------------------------- */
    
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
    /* These semaphores start as synchronization semaphores, including 
    our clock semaphore @49*/
        deviceSema4s[i] = 0;
    }

    /* ---------------- Instantiate a single process ---------------- */
    pcb_PTR p = allocPcb();
    /* If our current proc is not Null, then initalize all of currentProc's fields*/
    if(p != NULL){
        /*Set currentProc's state's stack pointer to our interval timer*/
        p->p_s.s_sp = (memaddr) RAMTOP;
        p->p_s.s_pc = p->p_s.s_t9 = (memaddr) test; /* test function in p2test */
        p->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
        p->p_supportStruct = NULL;
        /* Load interval Timer onto MILLI */
	LDIT(MILLI);
        /*Insert CurrentProc into our readyQue*/
        insertProcQ(&readyQue, p);
        processCnt += 1;
        p = NULL;
        /* Call the Scheduler */
        scheduler();
    }
    return 0;
}

/************************************************************************
General Exception Handler. Depending on the value obtained from the cause 
register, we pick 4 types of exception handling. 
************************************************************************/
void genExceptionHandler(){

    /* ------------------- Local Variables ------------------- */
    state_PTR state;
    int exeCause;
    /* ------------------------------------------------------- */
    
    /* The cause of our exception is obtained via the BIOSDATAPAGE */
    state = (state_t *) BIOSDATAPAGE;
    exeCause = (state->s_cause & GETEXECCODE) >> CAUSESHIFT;

    /* Depending on the value of our exception, we switch to the handler
    that corresponds to exeCause. */
    if(exeCause == INTERRUPTHANDLER){
        /* Cause of exception was an interupt. Jump to interrupt Handler */
        interruptHandler();    
    } else if(exeCause <= TLBEXCEPTS){
        /* Cause of exception was a TLB Refill event. Jump to TLB Handler */
        TLB_TrapHandler();      
    } else if(exeCause == SYSCALLEXECPTS){
        /* Cause of exception was a syscall. Jump to Syscall Handler */
        systemCall();              
    } else{
        /* Cause of exception was a program trap. Jump to TLB Trap Handler */
        programTRPHNDLR();
    }
}

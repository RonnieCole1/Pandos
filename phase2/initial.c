#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"

/* Declare global variables */
int procssCnt;      /* int indicating the number of strated, but not yet terminated processes */
int softBlockCnt;   /* number of started, but not terminated processes that are in the "blocked" statevdue to an I/O or timer request*/
pcb_t *readyQue;   /* tail pointer to a queue of pcbs that are in the "ready" state */
pcb_t *currentProc;  /* pointer to the pcb that is in the "running" state */
int deviceSema4s[MAXDEVICECNT]; 

int main(){
    /* Populate the Processor 0 Pass Up Vector */
    passupvector_t temp;
    temp->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    temp->tlb_refill_stackPtr = TOP;
    temp->exception_handler = (memaddr) fooBar;
    temp->exception_stackPtr = TOP;

    /* Initialize PCB and ASL data structures */
    initPcbs();
    initASL();

    /* Initialize all Nucleus maintained variables */
    procssCnt = softBlockCnt = 0;
    readyQue = mkEmptyProcQ();
    currentProc = NULL;
    int deviceSema4s[MAXDEVICECNT] = 0;

    /* Load system-wide Interval Timer with 100 milliseconds */
    INTERVALTMR
}
#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

/* global variables maintaining time usage*/
cpu_t TODStarted;
cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;

/*We need to implement a clock...*/

void scheduler() {

    state_t currentState;
    currentState = s_status;

    if(emptyProcQ(readyQue))
    {
        if(procssCnt == 0)
        {
            HALT();
        }
        else
        {
            if(softBlockCnt != 0)
            {
                /*Set status register to enable interupts and either
                disable the PLT or load it with a very large value*/

                WAIT();
            }
            else
            {
                /* deadlock */
                PANIC();
            }
        }
    }
    pcb_t *p
    Move_Process(p);

    /*Load 5ms on PLT*/
    Load_State(currentProc);     /*Load Processor State*/
}

Load_State(state_PTR currentProccess)
{
    SLVT(intervaltimer);
    setTimer(pseudoClockSema4);

    currentProc.p_s = currentProccess;
    LDST(&(currentProc.p_s));
}

myLDST(pcb_t *currProc){
    proc = currProc;
    LDST(&(currProc->p_s));
}

Move_Process(pcb_PTR p)
{
    removeProcQ(readyQue);
    insertChild(currentProc, p);
}

void uTLB_RefillHandler() {
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    Load_State((state_PTR) 0x0FFFF000);
}

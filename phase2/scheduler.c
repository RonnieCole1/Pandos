#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"



/*We need to implement a clock...*/

void scheduler() {

    state_t currentState;
    currentState = s_status;

    if(emptyProcQ(readyQue))
    {
        if(procssCnt == 0)
        {
            HAULT();
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
                PANIC();
            }
        }
    }
    pcb_t *p
    Move_Process(p);

    /*Load 5ms on PLT*/

    LoadState(currentProc) /*Load Processor State*/


}

Load_State(state_PTR currentProccess)
{
    SLVT(intervaltimer);
    setTimer(pseudoClockSema4);

    currentProc.p_s = currentProccess;
    LDST(&(currentProc.p_s));
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

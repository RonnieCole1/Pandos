#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../Phase2/initial.c"
#include "/usr/include/umps3/umps/libumps.h"



/*We need to implement a clock...*/

void scheduler() {

    state_t iniState;
    pcb_PTR p;

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
                
                currentProc = NULL;

                setTimer(DISABLE);

                iniState = ALLOFF | IEPON | IMON | TEBITON;
                LoadState(iniState);
                finalMSG("", FALSE);
            }
            else
            {
                finalMSG("", TRUE);
            }
        }
    }

    Move_Process(p);
    setTimer(TIMESLICE); /* Time slice is 5ms */
    LoadState(currentProc) /*Load Processor State*/
}

Load_State(state_PTR currentProccess)
{
    STCK(intervaltimer);
    setTimer(pseudoClockSema4);


    currentProc.p_s = currentProccess;
    LDST(&(currentProc.p_s));
}

/* Stealing this idea from Mikey. It seemed cool */
finalMSG(char msg[], bool Bstatus)
{
    if(Bstatus)
    {
        PANIC();
    }
    if(Bstatus == FALSE)
    {
        WAIT();
    }
    printf(char);
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

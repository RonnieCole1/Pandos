#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../Phase2/initial.c"
#include "../Phase2/scheduler.c"
#include "/usr/include/umps3/umps/libumps.h"

void SYSCALL(SYSNUM) 
{
    switch(SYSNUM) 
    {
        case CREATEPROCESS:
            Create_ProcessP();
        case TERMINATEPROCESS:
            Terminate_Process();
        case PASSEREN:
            wait(sema4);
        case VERHOGEN:
            signal(sema4);
        case WAITIO:
            Wait_for_IO_Device();
        case GETCPUTIME:
            Get_CPU_Time(p);
        case WAITCLOCK:
            Wait_For_Clock();
        case GETSUPPORTPRT:
            void Get_SUPPORT_Data();
    }

     Load_State(p);
}

/*SYS1*/
void Create_ProcessP()
{
    /*Initialize fields of p*/
    pcb_t *p;
    p->p_s = p->p_s.s_a1;
    p->p_supportStruct = s_a2;
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* Sys2 */
void Terminate_Process()
{
    while(emptyProcQ(currentProc) == FALSE)
    {
        Terminate_Process(removeChild(currentProc));
    }
    if(emptyChild(currentProc))
    {
        currentProc = currentProc->p_prnt
    }
    if()
    {

    } else {

    }
    currentProc = NULL;
}

/* Sys3 Passeren*/
pcb_t* wait(sema4)
{
    sema4--;
    if(sema4 < 0)
    {
        pcb_t *p = removeProcQ(&(sema4));
        insertBlocked(&(sema4),p);
    }
    BlockedSYS(p);
    return currentProc;
}

/* Sys4 Verhogen */
pcb_t* signal(sema4)
{
    sema4++;
    if(sema4 >= 0)
    {
        pcb_t *temp = removedBlocked(&(sema4));
        insertProcQ(temp, &(readyQue));
    }
    return currentProc;
}

/* Sys5 */
void Wait_for_IO_Device()
{
    BlockedSYS(currentProc);
    SYSCALL(PASSEREN, iosema4, 0, 0);
    /*Need to handle subdevices*/
}

/* Sys6 */
int Get_CPU_Time(pcb_t p)
{
    accumulatedTime = currentProc.p_time;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    wait(pseudoClockSema4);
    /*Do this every 100ms*/
    signal(pseudoClockSema4);
    /********************/
    BlockedSYS(currentProc);
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

/*Used for syscalls that block*/
void BlockedSYS(pcb_t p)
{
    /*Refer to 3.5.11 to complete this code */
    p.p_s.s_pc = p.p_s.s_pc + 4;
    p->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
    p->p_time = p->p_time + intervaltimer;
    insertBlocked(currentProc);
    scheduler()
    /****************************************/
}
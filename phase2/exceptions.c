#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "initial.c"

void exceptionHandler()
{
    if(/*Examine State Reg to check if in Kernal*/)
    {
        
    } else {
        /* I want to do this as a switch case... but how?*/
        /*Interupts*/
        if(Cause.ExcCode == 0)
        {

        }
        /*TLB Exceptions*/
        if(Cause.ExcCode <= 3 && Cause.ExcCode >= 1)
        {
            
        }
        /*Program Traps*/
        if((Cause.ExcCode <= 4 && Cause.ExcCode >= 7 )
        && (Cause.ExcCode <= 9 && Cause.ExcCode >= 12 ))
        {
            
        }
        /*SYSCALL*/
        if(Cause.ExcCode == 8)
        {
            /* How to define SYSCall?*/
            /*SYSCALL(x,x,x,x)*/
        }
    }
}
/*These fields don't match the book, but it
comes as close as I could...*/
void SYSCALL(int SYSNUM, int sema4, blah, blah) 
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

    LDST();
}

/*SYS1*/
void Create_ProcessP()
{
    /*Initialize fields of p*/
    pcb_t *p;
    /*Initialize fields of p*/
    p->p_s = s_a1;
    p->p_supportStruct = s_a2;
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* Sys2 */
void Terminate_Process()
{
    while(emptyChild(currentProc) == FALSE)
    {
        removeChild(currentProc);
        Terminate_Process();
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
    /*These need moved somewhere*/
    int accumulatedCPUTime;
    pcb_t *temp = currentProc->p_sibn;
    /******************************/
    while(temp->p_sibn != NULL)
    {
        accumulatedCPUTime += temp->p_time;
        if(emptyChild(temp) == FALSE)
        {
            while(temp->p_child->p_sibn != NULL)
            {
                accumulatedCPUTime += temp->p_child->p_time;
                Get_CPU_Time(*temp->p_child);
                temp->p_child = temp->p_child->p_sibn;
            }
        }
        temp = temp->p_next;
    }
    s_v0 = accumulatedCPUTime;
    return accumulatedCPUTime;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    SYSCALL(PASSEREN, pseudoClockSema4, 0, 0);
    /*Do this every 100ms*/
    SYSCALL(VERHOGEN, pseudoClockSema4, 0, 0);
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
    s_pc = s_pc + 4;
    p->p_s = /*BOISDATAPAGESTATE*/
    p->p_time = p->p_time + intervaltimer;
    insertBlocked(currentProc);
    scheduler()
    /****************************************/
}
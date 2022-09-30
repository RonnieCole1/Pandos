#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "initial.c"

/* Sys1 */
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

}

/* Sys6 */
int Get_CPU_Time()
{
    int accumulatedCPUTime;
    while(currentProc->p_next != NULL)
    {
        if(emptyChild(currentProc))
        {
            while(currentProc->p_child->p_sibn != NULL)
            {
                accumulatedCPUTime += currentProc->p_sibn->p_time;
                currentProc = currentProc->p_sibn;
            }
        }
        accumulatedCPUTime += currentProc->p_time;
        currentProc = currentProc->p_next
    }
    s_v0 = accumulatedCPUTime;
    return accumulatedCPUTime;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    wait(pseudoClockSema4);
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Exception Handling ****************************
 *
 *   WIP!
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

int sema4;

/* global variables from scheduler.c */
extern cpu_t TODStarted;
extern cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];

/* Syscall Handler. We take what is in register a_0 from the BIOSDATAPAGE, which is our sysnumber, and check its value from 1 to 8. If our
sysNum is greater than 8, we pass up or die.*/
void systemCall() {

    /* Set the a_0 register of the BIOSDATAPAGE to sysNum.*/
    int sysNum = ((state_t *) BIOSDATAPAGE)->s_a0;

    switch(sysNum) 
    {
        case CREATEPROCESS:
            Create_ProcessP();
            break;
        case TERMINATEPROCESS:
            Terminate_Process();
            break;
        case PASSEREN:
            wait();
            break;
        case VERHOGEN:
            signal();
            break;
        case WAITIO:
            Wait_for_IO_Device();
            break;
        case GETCPUTIME:
            Get_CPU_Time();
            break;
        case WAITCLOCK:
            Wait_For_Clock();
            break;
        case GETSUPPORTPRT:
            Get_SUPPORT_Data();
            break;
        default:
        /* If we reach here, our sysNum is not between 1-8. We passupordie*/
            passUpOrDie(GENERALEXCEPT);
    }
}

/* 
    System Call 1: When called, a newly populated pcb is placed on the Ready Queue and made a child
    of the Current Process. pcb_t p is the name for this new process, and its fields obtained from registers a1 and a2.
    Its cpu time is initialized to 0, and its semaphore address is set to NULL since it is in the ready
    state. 
*/
void Create_ProcessP(state_t *caller){
    /* Initialize fields of p */
    pcb_t *p = allocPcb();
    /*p->p_s = s_a1;*/
    p->p_supportStruct = p->p_s.s_a2;
    p->p_s.s_v0 = 0;
    /* Make p a child of currentProc and also place it on the ReadyQueue */
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    processCnt++;
    /* Set cpu time to 0 and semAdd to NULL */
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* System Call 2: When called, the executing process and all its progeny are terminated. */
void Terminate_Process(){
    if(emptyChild(currentProc)){        /* current process has no child */
        outChild(currentProc);
        freePcb(currentProc);
        --processCnt;
    } else {
        /* recursive call */
        sys2Help(currentProc);
    }
    currentProc = NULL;                 /* no current process anymore */
    scheduler();
}

/* Recursively removes all the children of head */
void sys2Help(pcb_PTR head){
    while(!emptyChild(head)){
        sys2Help(removeChild(head));
    }

    if(head->p_semAdd != NULL){
        int* sem = head->p_semAdd;
        outBlocked(head);
        if(sem >= &(deviceSema4s[0]) && sem <= &(deviceSema4s[MAXDEVICECNT - 1])){
            softBlockCnt--;
        } else{
            ++(*sem);                   /* increment semaphore */
        }
    } else if(head == currentProc){
        /* remove process from its parent */
        outChild(currentProc);
    } else{
        /* remove process from readyQue */
        outProcQ(&readyQue, head);
    }

    /* free after no more children */
    freePcb(head);
    --processCnt;
}

/* System Call 3: Preforms a "P" operation or a wait operation. The semaphore is decremented
and then blocked.*/
void wait()
{
    sysHelper(1);
    /*if(sema4 > 0){
        pcb_t *p = removeProcQ(&(sema4));
        insertBlocked(&sema4, currentProc);
    }*/
}

/* System Call 4: Preforms a "V" operation or a signal operation. The semaphore is incremented
and is unblocked/placed into the ReadyQue.*/
void signal()
{
    sysHelper(2);
    /*if(sema4 <= 0){
        pcb_PTR temp = removeBlocked(&sema4);
        insertProcQ(&readyQue, temp);
    }*/
}

/* Sys5 */
void Wait_for_IO_Device()
{
    int deviceNumber;
    int reg1 = currentProc->p_s.s_a1;
    int reg2 = currentProc->p_s.s_a2;
    int reg3 = currentProc->p_s.s_a3;
    deviceNumber = reg2 + (reg1 - DISKINT) * DEVPERINT;
    if(reg2 == reg3) {
        deviceNumber += DEVPERINT;
    }
    sysHelper(2);
    softBlockCnt++;
}

/* Sys6 */
int Get_CPU_Time(pcb_t *p)
{
    STCK(currentTOD);
    double accumulatedTime = currentProc->p_time;
    currentTOD = (currentTOD - TODStarted) + accumulatedTime;
    context_Switch(currentProc);
}

/* Sys7 */
void Wait_For_Clock()
{
    sysHelper(3);
    /*if(semPClock < 0){
        pcb_PTR temp = removeBlocked(&sema4);
        insertProcQ(&readyQue, temp);
        softBlockCnt++;*/
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

/*Used for syscalls that block*/
void BlockedSYS(pcb_t *p)
{
    cpu_t stopped;
    STCK(stopped);
    currentProc->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
    currentProc->p_time = currentProc->p_time + INTERVALTMR;
    p = p->p_semAdd;
    insertBlocked(p, currentProc);
    currentProc = NULL;
    scheduler();
}

void programTRPHNDLR() {
    passUpOrDie(GENERALEXCEPT);
}

void TLB_TrapHandler() {
    passUpOrDie(PGFAULTEXCEPT);
}

/* Passup Or Die */

void passUpOrDie(pcb_t currProc, int ExeptInt) {
    if(currProc.p_supportStruct == NULL) {
        Terminate_Process();
    }
    if(currProc.p_supportStruct != NULL) {
        passUp(ExeptInt);
    }
}

void passUp(int ExeptInt) {
    state_PTR tempstate = ((state_t *) BIOSDATAPAGE)->s_cause & GETEXECCODE;
    currentProc->p_supportStruct->sup_exceptState[ExeptInt] = *tempstate;
    LDCXT(currentProc->p_supportStruct->sup_exceptContext[ExeptInt].c_stackPtr,
    currentProc->p_supportStruct->sup_exceptContext[ExeptInt].c_status,
    currentProc->p_supportStruct->sup_exceptContext[ExeptInt].c_pc);
}

void sysHelper(int optType) {
	switch (optType)
	{
		case 0: /* P Operation */
        		sema4 = (int) currentProc->p_s.s_a1;
        		sema4++;
        		if(sema4 < 0) {
            			pcb_PTR temp = removeBlocked(&sema4);
            			insertProcQ(&readyQue, temp);
            			BlockedSYS(currentProc);
        		}
    		case 1: /* V Operation */
    			sema4 = (int) currentProc->p_s.s_a1;
        		sema4++;
        		if(sema4 <= 0) {
        		    	pcb_t *p = removeProcQ(&(sema4));
            			insertProcQ(&sema4, currentProc);
        		}
    		case 2: ;
        		int semPClock; 
        		semPClock = deviceSema4s[MAXDEVICECNT-1];
        		semPClock--;
        		if(semPClock < 0) {
            			softBlockCnt++;
            			BlockedSYS(currentProc);
        		}
        }
    	context_Switch(currentProc);
}

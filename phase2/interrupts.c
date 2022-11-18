#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "/usr/include/umps3/umps/libumps.h"

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];

/* global variables from scheduler.c */
extern cpu_t TODStarted;

void debug(int i){
    i = 4;
}

void interruptHandler(){
    /* local variables */
    unsigned int cause;
    cpu_t startTime, endTime;
    int devNum, lineNum, state, devSem;
    device_t *deviceReg;
    devregarea_t *devReg = (devregarea_t *) RAMBASEADDR;
    pcb_PTR temp;
    state_PTR oldInt = (state_PTR) BIOSDATAPAGE;
    int* semV;

    STCK(startTime);        /* save time started in interrupt handler */

    cause = oldInt->s_cause;

    /* single out possible interrupting lines */
    cause = cause >> 8;
    lineNum = 0;

    if((cause & SECOND) != 0){              /* local timer, line 1 */
        localTimer(startTime);
    } else if((cause & THIRD) != 0){        /* interval timer, line 2*/
        LDIT(MILLI);        /* load 100 ms into interval timer */
        semV = (int*) &(deviceSema4s[MAXDEVICECNT-1]);
        while(headBlocked(semV) != NULL){
            temp = removeBlocked(semV);
            STCK(endTime);
            if(temp != NULL){
                insertProcQ(&readyQue, temp);
                temp->p_time = (temp->p_time) + (endTime - startTime);
                softBlockCnt--;
            }
        }
        (*semV) = 0;
        localTimer(startTime);
    } else if((cause & FOURTH) != 0){       /* disk device */
        lineNum = DISKINT;
    } else if((cause & FIFTH) != 0){        /* flash device */
        lineNum = FLASHINT;
    } else if((cause & SIXTH) != 0){        /* network device */
        lineNum = NETWINT;
    } else if((cause & SEVENTH) != 0){      /* printer device */
        lineNum = PRNTINT;
    } else if((cause & EIGHTH) != 0){       /* terminal device */
        lineNum = TERMINT;
    } else{
        PANIC();                            /* interrupt caused for unknown reason */
    }

    /* get device number */
    devNum = getDeviceNumber((unsigned int*) (0x1000002C + ((lineNum - 3) * WORDLEN)));

    /* Non-Timer Interrupts */
    int devAddrBase = 0x10000054 + ((lineNum - 3) * EIGHTH) + (devNum * FIFTH);     /* POPs pg. 28 */ /* calculate address for device's device register */
    
    devSem = (((lineNum - 0x3) * DEVPERINT) + devNum);
    
    if(lineNum == TERMINT){
        volatile devregarea_t *devReg = (devregarea_t *) RAMBASEADDR;
        if((devReg->devreg[devSem].t_transm_status & 0x0F) != READY){
            state = devReg->devreg[devSem].t_transm_status;
            devReg->devreg[devSem].t_transm_command = ACK;
        } else{
            state = devReg->devreg[devSem].t_recv_status;
            devReg->devreg[devSem].t_recv_command = ACK;
            devSem = devSem + DEVPERINT;
        }
    } else{ 
        state = (devReg->devreg[devSem]).d_status;
        devReg->devreg[devSem].d_command= ACK;
    }

    deviceSema4s[devSem] += 1;

    if(deviceSema4s[devSem] <= 0){
        temp = removeBlocked(&(deviceSema4s[devSem]));
        temp->p_s.s_v0 = state;
        insertProcQ(&readyQue, temp);
        --softBlockCnt;
    }

    if(currentProc == NULL){
        scheduler();
    }
}

int getDeviceNumber(unsigned int* bitMap){
    unsigned int cause = *bitMap;
    if((cause & FIRST) != 0){
        return 0;
    } else if((cause & SECOND) != 0){
		return 1;
	} else if((cause & THIRD) != 0){
		return 2;
	} else if((cause & FOURTH) != 0){
		return 3;
	} else if((cause & FIFTH) != 0){
		return 4;
	} else if((cause & SIXTH) != 0){
		return 5;
	} else if((cause & SEVENTH) != 0){
		return 6;
	} else if((cause & EIGHTH) != 0){
		return 7;
	}
    return -1;
}

void localTimer(cpu_t startTime){
    cpu_t endTime;
    state_PTR oldInt = (state_PTR) BIOSDATAPAGE;
    if(currentProc != NULL){
        STCK(endTime);
        currentProc->p_time = currentProc->p_time + (endTime - TODStarted);
        copyState(oldInt, &(currentProc->p_s));
        insertProcQ(&readyQue, currentProc);
    }
    scheduler();
}

void copyState(state_PTR first, state_PTR copy) {
    int i;
    copy->s_entryHI = first->s_entryHI;
    copy->s_cause = first->s_cause;
    copy->s_status = first->s_status;
    copy->s_pc = first->s_pc;
    for (i = 0; i < STATEREGNUM; i++) {
        copy->s_reg[i] = first->s_reg[i];
    }
}

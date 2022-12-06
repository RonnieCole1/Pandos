#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "/usr/include/umps3/umps/libumps.h"

/* ---------------- External Global Variables ---------------- */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];
extern int* ClockSema4;
extern cpu_t TODStarted;

/************************************************************************
	From genExceptionHandler, we arive at interruptHandler() when the value
	of our Cause.ExeCode is 0. Global variables are initialized and we check
	the following:
		
		1. If our interrupt was caused by our timers.
		
		2. If we have a device interrupt.
		
	We handle each whichever interrupt we've recieved and return to our
	current process.
************************************************************************/
void interruptHandler(){

    /* local variables */
    unsigned int cause;
    cpu_t stopTOD;
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
        localTimer();
    } else if((cause & THIRD) != 0){        /* interval timer, line 2*/
    	LDIT(MILLI);
        STCK(stopTOD);
        pcb_PTR p = removeBlocked(ClockSema4);
        if (p != NULL) {
        	softBlockCnt--;
        }
        while (p != NULL){
            insertProcQ(&readyQue, p);
            p = removeBlocked(ClockSema4);
        }
        localTimer();
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
    /* Non-Timer Interrupts */
    if (lineNum >= 3) {
    	/* get device number */
    	devNum = getDeviceNumber((unsigned int*) (0x1000002C + ((lineNum - 3) * WORDLEN)));
    	int devAddrBase = 0x10000054 + ((lineNum - 3) * EIGHTH) + (devNum * FIFTH);     /* POPs pg. 28 */ /* calculate address for device's device register */
    	device_t *devReg = (device_t *) devAddrBase;
    	devSem = (((lineNum - 0x3) * DEVPERINT) + devNum);
    
    	if(lineNum == TERMINT){
        	if((devReg->t_transm_status & 0x0F) != READY){
            	state = devReg->t_transm_status;
            	devReg->t_transm_command = ACK;
        	} else{
            	state = devReg->t_recv_status;
            	devReg->t_recv_command = ACK;
            	devSem = devSem + DEVPERINT;
        	}
    	} else{ 
        	state = devReg->d_status;
        	devReg->d_command= ACK;
    	}

    	deviceSema4s[devSem] += 1;

    	if(deviceSema4s[devSem] <= 0){
        	temp = removeBlocked(&(deviceSema4s[devSem]));
        	STCK(stopTOD);
        	temp->p_s.s_v0 = state;
    		temp->p_time = (temp->p_time) +(stopTOD- TODStarted);
    		insertProcQ(&readyQue, temp);
    		--softBlockCnt;
    	}
    	localTimer();
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

void localTimer(){
    state_PTR oldInt = (state_PTR) BIOSDATAPAGE;
    if(currentProc != NULL){
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

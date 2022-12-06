#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Exception Handling **************************************
 *
 *   Interupts.c handles interupts from either timers or devices. Reguardless we always end 
 *   return to our current process. For timer interupts, we have PLT interupts and interval
 *   timer interupts. Our PLT interupts are handled by simply returning to our current
 *   process. Additional steps are taken for interval timer interupts. For device interupts, we
 *   first find the line number that corresponds to the correct device type. The semaphore and
 *   register address for this device is calculated. This device is acknowledged and if it
 *   was an interupt caused by our terminal, then we take some additional steps. The status
 *   code associated with this interupt is stored. This device semaphore is incremented and 
 *   a V operation is preformed on it. Our status code is placed into v0. In addition, if
 *   our current process was NULL, we call our scheduler.
 * 
 *	Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

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

    /* ------------------- Local Variables ------------------- */
    unsigned int cause;
    cpu_t startTime, endTime, stopTOD;
    int devNum, lineNum, state, devSem;
    device_t *deviceReg;
    devregarea_t *devReg;
    pcb_PTR temp;
    state_PTR oldInt;
    int* semV;
    device_t *devRegister;
    
    /* ------------- Initilize Local Variables ------------- */
    
    /* Pull the cause register from our BIOSDATAPAGE */
    oldInt = (state_PTR) BIOSDATAPAGE;
    cause = oldInt->s_cause;
    
    /* single out possible interrupting lines */
    cause = cause >> 8;
    
    /* Set devReg to RAMBASEADDR in preperation for our bitMap */
    devReg = (devregarea_t *) RAMBASEADDR;
    
    /* save time started in interrupt handler */
    STCK(startTime);
    
    /* Initilize our line number to 0 */
    lineNum = 0;
    
    /* Initilize our device register to NULL */
    devReg = NULL;

/************************************************************************
Input the value of our cause register and check  if the type of interrupt 
we encountered was the result of our Timeslice expiring or our Pseudo 
Clock. If our timers are not to blame, we now check if and what devices 
may have caused the interrupt.  These are our "non-timer interrupts".
************************************************************************/

/* ----------------- Timer Interrupt Checkers ----------------- */	

    /* If our PLT Timer is to blame, just return control to current
    process. */
    if((cause & SECOND) != 0){ 
        returnControltoCurrentProc();
        
    /* If our Pseudo/Interval Timer is to blame, do the following: */  
    } else if((cause & THIRD) != 0){  
        /* Load MILLI with our clock time */
    	LDIT(MILLI);
	/* Remove a pcb p associated with our blocked ClockSema4 */
        pcb_PTR p = removeBlocked(ClockSema4);
        
        /* If p is not NULL, we unblocked our semaphore. Decrement */
        if (p != NULL) {
        	softBlockCnt--;
        }
        
        /* While p is not NULL, add p to our readyQueue and keep
        unblocking our semaphore */
        while (p != NULL){
            insertProcQ(&readyQue, p);
            p = removeBlocked(ClockSema4);
        }
        
        /* Return control to current Process */
        returnControltoCurrentProc();
        
/************************************************************************
If we reach here, then the interupt was caused by a device. In which case,
we check our Disk, Flash, Network, Printer, and Terminal devices to see if
they are to blame for our interupt. If they are, we set our line number to
its corresponding device line number.
************************************************************************/      
/* ----------------- Device Interrupt Checkers ----------------- */	       
    
    /* If our Disk is to blame, set line number to DISKINT */
    } else if((cause & FOURTH) != 0){      
        lineNum = DISKINT;
    
    /* If our Flash device is to blame, set line number to FLASHINT */
    } else if((cause & FIFTH) != 0){       
        lineNum = FLASHINT;
        
    /* If our Network device is to blame, set line number to NETWINT*/
    } else if((cause & SIXTH) != 0){  
        lineNum = NETWINT;
    
    /* If our Printer device is to blame, set line number to PRNTINT*/  
    } else if((cause & SEVENTH) != 0){      
        lineNum = PRNTINT;
    
    /* If our Terminal device is to blame, set line number to TERMINT*/  
    } else if((cause & EIGHTH) != 0){ 
        lineNum = TERMINT;
    /* We should not reach this! Interrupt caused for unknown reason */
    } else{
        PANIC();                    
    }
    

/***********************************************************************
When handling device interrupts, there are a number of steps we take. We 
calculate the address for the interrupted device's device register. We 
find the device number of the device causing the interrupt. We obtain the 
device's semaphore. If this was a terminal device interrupt, there is an 
additonal step that is handled. In either case, we save the status code 
from this device and preform a V operation on the device's semaphore. We 
then return the status code in V0.
************************************************************************/
    if (lineNum >= DISKINT) {
    
    	/* Obtain our device number using our helper function */
    	devNum = getDeviceNumber((unsigned int*) (DEVCONST + ((lineNum - 3) * WORDLEN)));
    	
    	/* calculate address for device's device register */
    	int devAddrBase = REGCONST + ((lineNum - 3) * EIGHTH) + (devNum * FIFTH);
    	
    	/* Set our device's semaphore and register to the values we just obtained */
    	devRegister = (device_t *) devAddrBase;
    	devSem = (((lineNum - 3) * DEVPERINT) + devNum);
    
        /* Check if the interrupted device was Terminal */
    	if(lineNum == TERMINT){
        	if((devRegister->t_transm_status & TBITS) != READY){
            	state = devRegister->t_transm_status;
            	devRegister->t_transm_command = ACK;
        	} else{
            	state = devRegister->t_recv_status;
            	devRegister->t_recv_command = ACK;
            	devSem = devSem + DEVPERINT;
        	}
        }
        /* If it this was not a Terminal interrupt...*/
    	if(lineNum != TERMINT){
    	        /* set our status code to our device's status*/
        	state = devRegister->d_status;
        	/* Acknowledge the device! */
        	devRegister->d_command= ACK;
    	}
       
    	deviceSema4s[devSem] += 1;
    	
         /* V our device semaphore, and place our status code into v0*/
    	if(deviceSema4s[devSem] <= 0){
        	temp = removeBlocked(&(deviceSema4s[devSem]));
        	STCK(stopTOD);
        	temp->p_s.s_v0 = state;
    		temp->p_time = (temp->p_time) +(stopTOD- TODStarted);
    		insertProcQ(&readyQue, temp);
    		--softBlockCnt;
    	}
    	
    	/* Return control to current Process */
    	returnControltoCurrentProc();
    }
    
    /* If our current process is NULL, we need to call our scheduler
	and get a new one going */
    if(currentProc == NULL){
        scheduler();
    }
}

/***********************************************************************
Based on the value of bitMap and the address of the device, we check if 
it is on. If it is, we have found our device.
************************************************************************/
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
/***********************************************************************
After checking and preforming whatever operations from either handler, we 
want to return to our current process.
************************************************************************/
void returnControltoCurrentProc(){
    state_PTR oldInt = (state_PTR) BIOSDATAPAGE;
    if(currentProc != NULL){
        copyState(oldInt, &(currentProc->p_s));
        insertProcQ(&readyQue, currentProc);
    }
    scheduler();
}

/***********************************************************************
Parameters are pointers to both a "first" state and a "copy" state. The 
fields from "copy" are pasted into the "first" state aswell as all of the 
associated registers with that state.
************************************************************************/
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

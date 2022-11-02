#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "/usr/include/umps3/umps/libumps.h"
#include "p2test.c"

void interruptHandler(){

    cpu_t stopped;
    cpu_t remaining;
    STCK(stopped);
    remaining = getTIMER(); 

    if ((((state_PTR) BIOSDATAPAGE)->s_cause & PRNTINT) !=0){
        if(currentProc != NULL){
            currentProccess->p_time = currentProccess->p_time + (stopped-TODStarted);
            copyState(&(currentProccess->p_s),((state_PTR) BIOSDATAPAGE));
            insertProcQ(&readyQue, currentProc);
            scheduler();
        }
        else{
            PANIC();
        }
    }

    if((((state_PTR)BIOSDATAPAGE)->s_cause & 2) !=0){
        pcb_PTR temp;
        LDIT(100000);
        temp = removeBlocked(&semD[48]);
        while(temp != NULL){
            insertProcQ(&readyQue, temp);
            --softBlockCnt;
            temp = removeBlocked(&semD[48]);
        }
        semD[48] = 0;
        if(currentProc == NULL){
            scheduler();
        }
    }
    
    if((((state_PTR) BIOSDATAPAGE)->s_cause & DISKINT)!=0){
        devIntHelper(0x3);
    }
    if((((state_PTR) BIOSDATAPAGE)->s_cause & FLASHINT)!=0){
        devIntHelper(0x4);
    }
    if((((state_PTR) BIOSDATAPAGE)->s_cause & PRNTINT)!=0){     
        devIntHelper(0x6);
    }
    if((((state_PTR) BIOSDATAPAGE)->s_cause & TERMINT)!=0){
        devIntHelper(0x7);
    }
    if(currentProc != NULL){
        currentProccess->p_time = currentProccess->p_time + (stopped-TODStarted);
        copyState(&(currentProccess->p_s), ((state_PTR) BIOSDATAPAGE));
        readyTimer(currentProc, remaining);
    } else{
        HALT();
    }
}

void devIntHelper(int tempnum){
    pcb_PTR temp;
    unsigned int bitMap;
    devregarea_t* devReg;
    devReg= (devregarea_t*) RAMBASEADDR;
    bitMap = devReg->interrupt_dev[tempnum-DISKINT];
    int devSem;
    int devNum;
    int state;
               
    if((bitMap & 0x1) != 0){
        devNum = 0;
    } else if((bitMap & 0x2) != 0){
        devNum = 1;
    } else if((bitMap & 0x4) != 0){
        devNum = 2;
    } else if((bitMap & 0x8) != 0){
        devNum = 3;
    } else if((bitMap & 0x10) != 0){
        devNum = 4;
    } else if((bitMap & 0x20) != 0){
        devNum = 5;
    } else if((bitMap & 0x40) != 0){
        devNum = 6;
    } else{
        devNum = 7;
    }

    devSem = (((temp - 0x3)*DEVPERINT)+devNum);

    if(temp == TERMINT){
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

    semD[devSem] += 1;

    if(semD[devSem] <= 0){
        temp = removeBlocked(&(semD[devSem]));
        temp->p_s.s_v0 = state;
        insertProcQ(&readyQue, temp);
        --softBlockCnt;
    }

    if(currentProc == NULL){
        scheduler();
    }
}

/*
    Copies the processor state pointed to by first to the location pointed to by copy
*/
void copyState(state_PTR first, state_PTR copy) {
    int i;
    for (i = 0; i < STATEREGNUM; i++) {
        copy->s_reg[i] = first->s_reg[i];
    }
    copy->s_entryHI = first->s_entryHI;
    copy->s_cause = first->s_cause;
    copy->s_status = first->s_status;
    copy->s_pc = first->s_pc;        
}

void readyTimer(pcb_PTR cp, cpu_t time){
    STCK(TODStarted);                                                                                                                      
    setTIMER(time);                                                                                                
    contSwitch(cp);                                
}

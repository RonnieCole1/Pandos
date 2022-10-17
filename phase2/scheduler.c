#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"
#include "initial.c"


void scheduler() {
    if(emptyProcQ(readyQue))
    {
        if(procssCnt == 0){
            /*Invoke HAULT*/
        }
        if(procssCnt > 0 && softBlockCnt > 0)
        {
            /*set Status reg to enable interupts and
            disable the PLT or load it with a very large
            value */
            /*Invoke WAIT*/
        }
    }
    pcb_t *p = removeProcQ(readyQue);
    insertChild(currentProc, p);

    /*Load 5ms on PLT*/

    LDST(p->p_s) /*Load Processor State*/


}
int emptyProcQ(pcb_t *tp) {
    return(tp == NULL);
}

pcb_t *headProc(pcb_t *tp) {
    if(emptyProcQ(tp)) {
        return NULL;
    }
    return (tp -> p_next);
}

pcb_t *mkEmptyProcQ() {
    return NULL;
}

void insertProcQ(pcb_t **tp, pcb_t *p) {
    if (emptyProc(tp) {
        *(tp) = p;
        p -> p_next = p;
        p -> p_prev = p;
        return;
    }
    *(tp) = R;
    R -> p_next = Q;
    R -> p_prev = N;
    R -> p_next = N;
    N -> p_prev = Q;
    N -> p_next = Q;
    R -> p_next = NULL;
    R -> p_prev = NULL;

}

initPcbs() {
    static pcb_t initArray[MAXPCBCNT];
    for (int i = 0; i < MAXPCBCNT; i++) {
        allocPcbs(&initArray[i]);
    }
}

pcb_t *removeProcQ(pcb_t **tp) {
    if (emptyProcQ(tp)) {
        return NULL;
    } else {
        *(tp) = P;
        pcb_t temp = p -> p_prev;
        temp = temp -> p_next;
        p = NULL;
        p -> p_prev = Q;
        p -> p_next = R;
        Q -> p_next = R;
        R -> p_prev = Q;
        if (tp -> p_next == p) {
            tp -> p_next = Q;
        }
        return (temp);
    }
}

/*         Trees                 */

int emptyChild(pcb_t *p) {
    return(p -> p_child = NULL);
}

void insertChild (pcb_t *prnt, pcb_t *p){
    p -> p_prnt = prnt;
    prnt -> p_child = p;
}
        
pcb_t *removeChild(pcb_t){
    p->p_child = null;
    if(p->p_prnt == null){
        return null;
    } else{
        return(p->p_child);
    }

#pragma once
#include <stdlib.h>

// - Resource initialization function
//      * Will be called only once at the initialization phase
// - mem_size: physical memory size in bytes
//      * You need to allocate a memory space and manage alloc/free lists 
//         – Assume that page frame 0 is occupied by OS
//      *  Do not consider the memory space consumed by page table and internal data structures 
//         (e.g., alloc/free lists and PCBs)
// - swap_size: swap disk size in bytes
//      * Allocate a memory space instead of real disk space
//      * You may need to manage alloc/free lists
// - pmem and swap spaces are dummy; no need of actual data copies between pmem and swap spaces
// - Return value
//      * Pointer (i.e., address) to the allocated memory area that simulates the
//      * physical memory • 0: fail

typedef struct ku_mmu_pfn {
    int pfn;
    struct ku_mmu_pfn *next;
} PFN;

typedef struct ku_mmu_AFList {
    PFN *p_free_head;
    PFN *p_free_tail;

    PFN *p_alloc_head;
    PFN *p_alloc_tail;

    PFN *s_free_head;
    PFN *s_free_tail;

    PFN *s_alloc_head;
    PFN *s_alloc_tail;
} ALLOC_FREE_LIST;

void *ku_mmu_swapSpace;
ALLOC_FREE_LIST ku_mmu_afList;

void push_AFList_PFN(PFN *head, PFN *tail, PFN *insert) {
    if (head == NULL) {
        head = tail = insert;
        return;
    }

    tail->next = insert;
    tail = insert;
}

PFN *pop_AFList_PFN(PFN *head, PFN *tail) {
    if (head == NULL) {
        return 0;
    }

    PFN *ret = head;
    head = ret->next;
    ret->next = NULL;

    return ret;
}

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
    void *pmem = malloc(sizeof(char) * mem_size);
    if (pmem = NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Physcal Memory Allocation Failed\n");
        return 0;
    }

    void *swap = malloc(sizeof(char) * swap_size);
    if (swap = NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Swap Space Memory Allocation Failed\n");
        return 0;
    }
    ku_mmu_swapSpace = swap;

    ku_mmu_afList.p_free_head = ku_mmu_afList.p_free_tail = NULL;
    ku_mmu_afList.p_alloc_head = ku_mmu_afList.p_alloc_tail = NULL;
    ku_mmu_afList.s_free_head = ku_mmu_afList.s_free_tail = NULL;
    ku_mmu_afList.s_alloc_head = ku_mmu_afList.s_alloc_tail = NULL;

    int i = 0;
    // Physical Memory freeList
    for (i = 1; i < mem_size / 4; i++) {
        PFN *npfn = (PFN *) malloc(sizeof(PFN));
        if (npfn == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Pysical Memory FreeList Not Success.\n");
        }
        npfn->pfn = i; npfn->next = NULL;
        push_AFList_PFN(ku_mmu_afList.p_free_head, ku_mmu_afList.p_free_tail, npfn);
    }

    // Physcial Memory allocList
    PFN *osPfn = (PFN *) malloc(sizeof(PFN));
    if (osPfn == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Make Pysical Memory AllocList Not Success.\n");
    }
    osPfn->pfn = 0; osPfn->next = NULL;
    push_AFList_PFN(ku_mmu_afList.p_alloc_head, ku_mmu_afList.p_alloc_tail, osPfn);

    // Swap Memory freeList
    for (i = 0; i < swap_size / 4; i++) {
        PFN *spfn = (PFN *) malloc(sizeof(PFN));
        if (spfn == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Swap Memory FreeList Not Success.\n");
        }
        spfn->pfn = i; spfn->next = NULL;
        push_AFList_PFN(ku_mmu_afList.s_free_head, ku_mmu_afList.s_free_tail, spfn);
    }

    return pmem;
}

// – Performs context switch
//      * If pid is new, the function creates a process (i.e., PCB) and its page table
// – pid: pid of the next process
// – ku_cr3: stores the base address of the page table for the current process
//      * Points an 8-bit PTE (i.e., base address of the page table)
//      * Should be changed by this function when a context switch happens 
//      * struct ku_pte should be defined appropriately
// – Return value 
//      * 0: success
//      * -1: fail

struct ku_pte {

};

int ku_run_proc(char pid, struct ku_pte **ku_cr3) {

}
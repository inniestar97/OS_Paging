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

typedef struct mem_info {
    int mem_num;
    struct mem_info *next;
} MEM_INFO;

typedef struct mem_list {
    MEM_INFO *head;
    MEM_INFO *tail;
} MEM_LIST;

void *ku_mmu_swapSpace;

MEM_LIST *ku_mmu_p_freeList;
MEM_LIST *ku_mmu_p_allocList;
MEM_LIST *ku_mmu_s_freeList;
MEM_LIST *ku_mmu_s_allocList;

char *ku_mmu_pageTable;

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
    mem_size -= mem_size % sizeof(int);
    swap_size -= swap_size % sizeof(int);
    int *pmem = (int *) malloc(mem_size * sizeof(char));
    if (pmem == NULL) {
        fprintf(stderr, "Physical Memory Allocation Failed\n");
        return 0;
    }

    ku_mmu_swapSpace = malloc(mem_size * sizeof(char));
    if (ku_mmu_swapSpace == NULL) {
        fprintf(stderr, "SwapSpace Memory Allocation Failed\n");
        return 0;
    }

    ku_mmu_pageTable = (char *) calloc((mem_size / 4), sizeof(char));
    // ku_mmu_pageTable = (char *) malloc((mem_size / 4) * sizeof(char));
    // memset(ku_mmu_pageTable, 0, (mem_size / 4) * sizeof(char));
    if (ku_mmu_pageTable = NULL) {
        fprintf(stderr, "PageTable Memory Allocation Failed\n");
        return 0;
    }

    ku_mmu_p_freeList = (MEM_LIST *) malloc(sizeof(MEM_LIST));
    ku_mmu_p_freeList->head = ku_mmu_p_freeList->tail = NULL;
    ku_mmu_p_allocList = (MEM_LIST *) malloc(sizeof(MEM_LIST));
    ku_mmu_p_allocList->head = ku_mmu_p_allocList-> tail = NULL;
    ku_mmu_s_freeList = (MEM_LIST *) malloc(sizeof(MEM_LIST));
    ku_mmu_s_freeList->head = ku_mmu_s_freeList-> tail = NULL;
    ku_mmu_s_allocList = (MEM_LIST *) malloc(sizeof(MEM_LIST));
    ku_mmu_s_allocList->head = ku_mmu_s_allocList-> tail = NULL;

    if (ku_mmu_p_freeList == NULL || ku_mmu_p_allocList == NULL
            || ku_mmu_s_freeList == NULL || ku_mmu_s_allocList == NULL) {
        fprintf(stderr, "FREE_ALLOC List Memory Allocation Failed\n");

        return 0;
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
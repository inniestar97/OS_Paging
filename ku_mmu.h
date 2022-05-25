#pragma once
#include <stdlib.h>
#include <stdio.h>

typedef struct ku_mmu_page_frame {
    int pfn;
    struct ku_mmu_page_frame *next;
} PAGE_FRAME;

typedef struct ku_mmu_alloc_free_list {
    PAGE_FRAME *head;
    PAGE_FRAME *tail;
} ALLOC_FREE_LIST;

// 스왑공간 시작 포인터 (동적할당)
void *ku_mmu_swap_space;

// 메모리, 스왑공간 alloc, free list
ALLOC_FREE_LIST *ku_mmu_mem_free_list;
ALLOC_FREE_LIST *ku_mmu_mem_alloc_list;
ALLOC_FREE_LIST *ku_mmu_swap_free_list;
ALLOC_FREE_LIST *ku_mmu_swap_alloc_list;

// alloc list 와 free list를 효과적으로 관리하기 위한 함수
// pushPageFrame, popPageFrame >> FIFO 방식으로 불러온다
void pushPageFrame(ALLOC_FREE_LIST *list, PAGE_FRAME *pf) {
    if (list->head == NULL) {
        list->head = list->tail = pf;
        return;
    }

    list->tail->next = pf;
    list->tail = pf;
}

PAGE_FRAME *popPageFrame(ALLOC_FREE_LIST *list) {
    if (list->head == NULL) {
        return 0;
    }

    PAGE_FRAME *ret = list->head;
    list->head = ret->next;
    ret->next = NULL;

    return ret;
}

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
    void *pmem = malloc(sizeof(char) * mem_size); // 메모리 공간 설정
    if (pmem == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Physical Memory Allocation Failed.\n");
        return 0;
    }

    void *swap = malloc(sizeof(char) * swap_size); // 스왑 공간 설정
    if (swap == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Swap space Memory Allocation Failed.\n");
        return 0;
    }
    ku_mmu_swap_space = swap;

    // alloc free List Initailzation
    ku_mmu_mem_free_list = (ALLOC_FREE_LIST *) malloc(sizeof(ALLOC_FREE_LIST));
    ku_mmu_mem_alloc_list = (ALLOC_FREE_LIST *) malloc(sizeof(ALLOC_FREE_LIST));
    ku_mmu_swap_free_list = (ALLOC_FREE_LIST *) malloc(sizeof(ALLOC_FREE_LIST));
    ku_mmu_swap_alloc_list = (ALLOC_FREE_LIST *) malloc(sizeof(ALLOC_FREE_LIST));

    if (ku_mmu_mem_free_list == NULL || ku_mmu_mem_alloc_list == NULL 
            || ku_mmu_swap_free_list == NULL || ku_mmu_swap_alloc_list == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: alloc_free_list Allocation Failed.\n");
        return 0;
    }

    ku_mmu_mem_free_list->head = ku_mmu_mem_free_list->tail = NULL;
    ku_mmu_mem_alloc_list->head = ku_mmu_mem_alloc_list->tail = NULL;
    ku_mmu_swap_free_list->head = ku_mmu_swap_free_list->tail = NULL;
    ku_mmu_swap_alloc_list->head = ku_mmu_swap_alloc_list->tail = NULL;

    int i = 0;
    // Physical Memory freeList
    // OS 가 항상 0번째 physical Frame 을 사용한다고 가정
    // 처음엔 모두 비여있으므로 ku_mmu_mem_free_list에 추가
    for (i = 1; i < mem_size / 4; i++) {
        PAGE_FRAME *pf = (PAGE_FRAME *) malloc(sizeof(PAGE_FRAME));
        if (pf == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Physical Memory FreeList Failed.\n");
            return 0;
        }
        pf->pfn = i; pf->next = NULL;
        pushPageFrame(ku_mmu_mem_free_list, pf);
    }


    // Swap Space Memory FreeList
    // 처음엔 모두 비여있으므로 ku_mmu_swap_free_list 에 추가
    for (i = 0; i < swap_size / 4; i++) {
        PAGE_FRAME *pf = (PAGE_FRAME *) malloc(sizeof(PAGE_FRAME));
        if (pf == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Swap Space Memory FreeList Failed.\n");
            return 0;
        }
        pf->pfn = i; pf->next = NULL;
        pushPageFrame(ku_mmu_swap_free_list, pf);
    }

    return pmem;
}

// 이진트리를 위한 process control block 구조체 생성
typedef struct ku_mmu_process_control_block {
    char pid;
    char table[64];
    struct ku_mmu_process_control_block *left;
    struct ku_mmu_process_control_block *right;
} PCB;

PCB *ku_mmu_root_pcb = NULL;

// 이진트리를 이용하여 pcb 접근.
// 이번 프로젝트에선 한번 생성된 프로세스는 삭제되지 않는다.
PCB *insertProcess(PCB *parent, int pid) {
    if (parent == NULL) {
        parent = (PCB *) calloc(1, sizeof(PCB));
        parent->left = parent->right = NULL;
        parent->pid = pid;
        return parent;
    } else {
        if (pid < parent->pid) {
            return insertProcess(parent->left, pid);
        } else {
            return insertProcess(parent->right, pid);
        }
    }
}

PCB *searchProcess(PCB *parent, int pid) {
    if (parent->pid == pid) {
        return parent;
    } else {
        if (pid < parent->pid) {
            return searchProcess(parent->left, pid);
        } else {
            return searchProcess(parent->right, pid);
        }
    }
}

int ku_run_proc(char pid, struct ku_pte **ku_cr3) {
    PCB *process = searchProcess(ku_mmu_root_pcb, pid);

    if (process == NULL) { // 기존에 존재했던 process 가 아니라면
        process = insertProcess(ku_mmu_root_pcb, pid);
        if (process == NULL) {
            fprintf(stderr, "ERROR- ku_run_proc: Process Control Block is not maked.\n");
            return -1;
        }
    }

    *ku_cr3 = (struct ku_pte *) process->table;
    return 0;
}

// TODO: ku_page_fault 처리 해야할것
int ku_page_fault(char pid, char va) {

}
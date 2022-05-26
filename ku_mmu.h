#pragma once
#include <stdlib.h>
#include <stdio.h>

typedef struct ku_mmu_page_frame {
    char pfn;
    char pid; // 해당 page frame에게 할당된 pid, 0일경우 어느 프로세스도 접근 안돼있다
    char va; // pid프로세스 내에서 해당 페이지가 가진 virtual address 0일경우 free
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

PAGE_FRAME *popPageFrame2(ALLOC_FREE_LIST *list, char pfn) {
    PAGE_FRAME *ret;

    PAGE_FRAME *pop = popPageFrame(list);
    while (pop != 0) {
        if (pop->pfn != pfn) {
            ret = pop;
        } else {
            pushPageFrame(list, pop);
        }
        pop = popPageFrame(list);
    }

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
        pf->pid = 0;
        pf->va = 0;
        pf->pfn = i;
        pf->next = NULL;
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
        pf->pid = 0;
        pf->va = 0;
        pf->pfn = i;
        pf->next = NULL;
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
    // 현재 프로세스가 처음 실행되었거나, swap space에 있는경우 present bit == 0이다.
    // 만약 swap space 에 있다면 swap space에서 physcial memory로 가져와야하고,
    // 처음 시작이라면 physical memory에서 하나를 할당시켜야한다

    PCB *current_process_pcb = searchProcess(ku_mmu_root_pcb, pid);
    char *pdbr = current_process_pcb->table;
    char offset = (va & 0xFC) >> 2;
    
    if (pdbr[offset]) { // true이면 swap Space에 존재한다
        char swap_page_offset = (pdbr[offset] & 0xFD) >> 1;

        // alloc되어있는 swap space의 해당 page을 꺼내온다
        PAGE_FRAME *asp = popPageFrame2(ku_mmu_swap_alloc_list, swap_page_offset);
        // -> swapspace를 건드린다는 얘기는 physical memory가 꽉 찼다는 얘기
        PAGE_FRAME *app = popPageFrame(ku_mmu_mem_alloc_list);

        asp->pid = app->pid;
        asp->va = app->va;
        app->pid = pid;
        app->va = va;

        PCB *to_swap = searchProcess(ku_mmu_root_pcb, asp->pid);
        char pte_offset = (asp->va & 0xFC) >> 2;
        to_swap->table[pte_offset] = asp->pfn << 1;

        char pte = (app->pfn << 2);
        pte = pte | 0x01;
        pdbr[offset] = pte;

        pushPageFrame(ku_mmu_swap_alloc_list, asp);
        pushPageFrame(ku_mmu_mem_alloc_list, app);

    } else { // 새로 만들어져서 swap space에도 존재하지 않는다.
        // 1. physical free 되어있는 page가 존재하는지 확인한다.
        // 2. 존재한다면 바로 사용
        // 3. 존재하지 않는다면 alloc되어있는 physical mem 중 하나를 swap으로이동.
        while (1) {
            PAGE_FRAME *fpp = popPageFrame(ku_mmu_mem_free_list);
            if (fpp != 0) { // physical free page is existed
                fpp->pid = pid;
                fpp->va = va;

                char pte = fpp->pfn << 2;
                pte = pte | 0x01;
                pdbr[offset] = pte;
                pushPageFrame(ku_mmu_mem_alloc_list, fpp);

                break;
            } else { // physical free page is not existed
                // Therefore one of the alloc page get free and to swap
                PAGE_FRAME *app = popPageFrame(ku_mmu_mem_alloc_list);
                char to_swap_pid = app->pid;
                char to_swap_va = app->va;

                app->pid = 0;
                app->va = 0;
                pushPageFrame(ku_mmu_mem_free_list, app);

                PAGE_FRAME *fsp = popPageFrame(ku_mmu_swap_free_list);
                fsp->pid = to_swap_pid;
                fsp->va = to_swap_va;
                PCB *to_swap = searchProcess(ku_mmu_root_pcb, to_swap_pid);

                char pte_offset = (va & 0xFC) >> 2;
                to_swap->table[pte_offset] = fsp->pfn << 1;
            }
        }
    }



    // PCB *pcb = searchProcess(ku_mmu_root_pcb, pid);
    // char offset = (va & 0xFC) >> 2;

    // PAGE_FRAME *pf = popPageFrame(ku_mmu_mem_free_list);
    // if (pf != 0) { // physical Memory에 아직 들어갈 수 있다면
    //     pf->pid = pid;
    //     pushPageFrame(ku_mmu_mem_alloc_list, pf);

    //     char pfn = pf->pfn;
    //     char pte = pfn << 2; // BBBB00
    //     pte = pte | 0x03; // BBBB01
    //     pcb->table[offset] = pte;

    // } else { // physcial Memory에 들어갈 수 없다면
    //     // alloc에서 free로 된 pf를 해당 프로세스에게 할당한다

    //     // free 시켜야할 physcial memory의 page frame
    //     PAGE_FRAME *to_free_pf = popPageFrame(ku_mmu_mem_alloc_list);

    //     PCB *to_free_pcb = searchProcess(ku_mmu_root_pcb, to_free_pf->pid);




    //     char to_swap_pid = to_free_pf->pid;
    //     to_free_pf->pid = pid;

    // }

}
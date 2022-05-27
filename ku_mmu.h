#pragma once
#include <stdlib.h>
#include <stdio.h>

typedef struct ku_mmu_page {
    char pfn;
    char pid;
    char va;
    struct ku_mmu_page *next;
} PAGE;

typedef struct ku_mmu_page_list {
    PAGE *head;
    PAGE *tail;
} PAGE_LIST;

PAGE_LIST *ku_mmu_fMem;
PAGE_LIST *ku_mmu_aMem;
PAGE_LIST *ku_mmu_fSwap;
PAGE_LIST *ku_mmu_aSwap;

// alloc or free list 의 tail 에 page삽입 -> physical, swapspace
void pushPage_toList(PAGE_LIST *list, PAGE *page) {
    if (list->head == NULL) {
        list->head = list->tail = page;
        return;
    }

    list->tail->next = page;
    list->tail = page;
}

PAGE_LIST *ku_mmu_PF_List; // physical memory page free list
PAGE_LIST *ku_mmu_PA_List; // physical memory page alloc list
PAGE_LIST *ku_mmu_SF_List; // swap space page free list
PAGE_LIST *ku_mmu_SA_List; // swap space page alloc list

// alloc or free list 에서 page를 가져온다.
// pfn < 0 이라면 가장 앞부분에서 page pop
// pfn >= 0 이라면 pfn에 해당하는 page pop
PAGE *popPage_fromList(PAGE_LIST *list, char pfn) {

    if (list->head == NULL) {
        return 0;
    }

    PAGE *ret;
    if (pfn < 0 || (list->head->pfn == pfn)) {
        ret = list->head;
        list->head = ret->next;
        ret->next = NULL;
    } else {
        PAGE *beforePage = NULL;
        PAGE *currentPage = list->head; 

        while (currentPage != NULL) {
            if (currentPage->pfn == pfn) {
                beforePage->next = currentPage->next;
                currentPage->next = NULL;

                ret = currentPage;
                break;
            }

            beforePage = currentPage;
            currentPage = currentPage->next;
        }
    }

    return ret;
}

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
    void *pmem = malloc(sizeof(char) * mem_size);
    if (pmem == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Physical Memory Allocation Failed.\n");
        return 0;
    }

    void *swap = malloc(sizeof(char) * swap_size); // 스왑 공간 설정
    if (swap == NULL) {
        fprintf(stderr, "ERROR- ku_mmu_init: Swap space Memory Allocation Failed.\n");
        return 0;
    }

    ku_mmu_PF_List = (PAGE_LIST *) malloc(sizeof(PAGE_LIST));
    ku_mmu_PA_List = (PAGE_LIST *) malloc(sizeof(PAGE_LIST));
    ku_mmu_SF_List = (PAGE_LIST *) malloc(sizeof(PAGE_LIST));
    ku_mmu_SA_List = (PAGE_LIST *) malloc(sizeof(PAGE_LIST));

    if (ku_mmu_PF_List == 0 || ku_mmu_PA_List == 0 || ku_mmu_SF_List == 0 || ku_mmu_SA_List == 0) {
        fprintf(stderr, "ERROR- ku_mmu_init: alloc_free_list Allocation Failed.\n");
        return 0;
    }

    ku_mmu_PF_List->head = ku_mmu_PF_List->tail = NULL;
    ku_mmu_PA_List->head = ku_mmu_PA_List->tail = NULL;
    ku_mmu_SF_List->head = ku_mmu_SF_List->tail = NULL;
    ku_mmu_SA_List->head = ku_mmu_SA_List->tail = NULL;

    int i = 0;
    for (i = 1; i < mem_size / 4; i++) {
        PAGE *page = (PAGE *) malloc(sizeof(PAGE));
        if (page == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Physical Memory FreeList Failed.\n");
            return 0;
        }
        page->pfn = i;
        page->pid = 0;
        page->va = 0;
        page->next = NULL;
        pushPage_toList(ku_mmu_PF_List, page);
    }

    for (i = 0; i < swap_size; i++) {
        PAGE *page = (PAGE *) malloc(sizeof(PAGE));
        if (page == NULL) {
            fprintf(stderr, "ERROR- ku_mmu_init: Make Physical Memory FreeList Failed.\n");
            return 0;
        }
        page->pfn = i;
        page->pid = 0;
        page->va = 0;
        page->next = NULL;
        pushPage_toList(ku_mmu_SF_List, page);
    }

    return pmem;
}

typedef struct ku_mmu_process_control_block {
    char pid;
    // 8bit addressing, 6bit: cr3_offset => 64, 1byte pte => char
    char table[64]; 
    struct ku_mmu_process_control_block *left;
    struct ku_mmu_process_control_block *right;
} PCB;

// 이진트리를 이용하여 찾는다. 루트 PCB
PCB *ku_mmu_rootPCB = NULL;

PCB *insertPCB(PCB *parent, char pid) {
    if (parent == NULL) {
        parent = (PCB *) malloc(sizeof(PCB));

        parent->left = parent->right = NULL;
        parent->pid = pid;

        int i = 0;
        for (i = 0; i < 64; i++) {
            parent->table[i] = 0;
        }

    } else{
        if (pid < parent->pid) {
            parent->left = insertPCB(parent->left, pid);
        } else {
            parent->right = insertPCB(parent->right, pid);
        }
    }

    return parent;
}

PCB *searchPCB(PCB *root, char pid) {
    if (root == NULL || root->pid == pid) {
        return root; 
    } 

    if (pid < root->pid) {
        return searchPCB(root->left, pid);
    }

    return searchPCB(root->right, pid);
}

int ku_run_proc(char pid, struct ku_pte **ku_cr3) {
    PCB *pcb = searchPCB(ku_mmu_rootPCB, pid);
    if (pcb == 0) {
        pcb = insertPCB(ku_mmu_rootPCB, pid);
        if (pcb == NULL) {
            fprintf(stderr, "ERROR- ku_run_proc: Process Control Block is not maked.\n");
            return -1;
        }
    }

    // pcb안의 table이 pte가 있는 table 의 시작주소
    *ku_cr3 = (struct ku_pte *) pcb->table;
    return 0;
}

int ku_page_fault(char pid, char va) {
    // 현재 돌려야하는 프로세스가 physical memory에 할당되어있지 않을때.
    PCB *current_proPCB = searchPCB(ku_mmu_rootPCB, pid);
    char *cr3 = current_proPCB->table;
    char pte_offset = (va & 0xFC) >> 2;
    char *pte = cr3 + pte_offset;

    if (*pte) { // swap space에 존재했었다면.
        // 현재 physical memory 가 full이라는 의미

        // 할당되어있전 swap공간을 해제시켜준다
        char swap_pfn = (*pte & 0xFD) >> 1;
        PAGE *swap_page = popPage_fromList(ku_mmu_SA_List, swap_pfn);
        swap_page->pid = 0;
        swap_page->va = 0;
        pushPage_toList(ku_mmu_SF_List, swap_page);
        swap_page = NULL;

        // 할당되어 있던 physicalmemory에서 가장 앞쪽을 (FIFO)
        // free 해주고, 해당 프로세스를 swap공간으로 이동한다.
        swap_page = popPage_fromList(ku_mmu_SF_List, -1);
        PAGE *mem_page = popPage_fromList(ku_mmu_PA_List, -1);

        PCB* mem_pagePCB = searchPCB(ku_mmu_rootPCB, mem_page->pid);
        char mem_page_pte_offset = (mem_page->va & 0xFC) >> 2;
        mem_pagePCB->table[mem_page_pte_offset] = swap_page->pfn << 1;
        swap_page->pid = mem_page->pid;
        swap_page->va = mem_page->va;
        pushPage_toList(ku_mmu_SA_List, swap_page);

        mem_page->pid = pid;
        mem_page->va = va;
        *pte = (mem_page->pfn << 2) + 1;
        pushPage_toList(ku_mmu_PA_List, mem_page);
    } else { 
        // 새로만들어져서 swap space 에도 존재하지 않았었다면
        // 아예 physical memory에 할당조차 되지 않았었다면
        // 1. physical memory free list에 page가 존재한다.
        // 2. physical memory free list에 page가 존재하지 않는다.
        PAGE *freePage = popPage_fromList(ku_mmu_PF_List, -1);

        if (freePage) { // 1. 존재한다면
            freePage->pid = pid;
            freePage->va = va;
            *pte = (freePage->pfn << 2) + 1;
            pushPage_toList(ku_mmu_PA_List, freePage);
        } else { // 2. 존재하지 않는다면
            PAGE *swap_page = popPage_fromList(ku_mmu_SF_List, -1);
            PAGE *mem_page = popPage_fromList(ku_mmu_PA_List, -1);

            PCB * mem_pagePCB = searchPCB(ku_mmu_rootPCB, mem_page->pid);
            char mem_page_pte_offset = (mem_page->va & 0xFC) >> 2;
            mem_pagePCB->table[mem_page_pte_offset] = swap_page->pfn << 1;
            swap_page->pid = mem_page->pid;
            swap_page->va = mem_page->va;
            pushPage_toList(ku_mmu_SA_List, swap_page);

            mem_page->pid = pid;
            mem_page->va = va;
            *pte = (mem_page->pfn << 2) + 1;
            pushPage_toList(ku_mmu_PA_List, mem_page);
        }
    }

    return 0;
}
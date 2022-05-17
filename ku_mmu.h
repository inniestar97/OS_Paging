
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

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {

}


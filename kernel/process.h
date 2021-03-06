#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"
typedef struct trapframe {
  // space to store context (all common registers)
  /* offset:0   */ riscv_regs regs;

  // process's "user kernel" stack
  /* offset:248 */ uint64 kernel_sp;
  // pointer to smode_trap_handler
  /* offset:256 */ uint64 kernel_trap;
  // saved user process counter
  /* offset:264 */ uint64 epc;

  //kernel page table
  /* offset:272 */ uint64 kernel_satp;
}trapframe;
typedef struct block
{
	uint64 va;
	uint64 pa;
	uint32 valid;	//0->available
	uint64 size;
}block;
#define Max_block 5
typedef struct page{
	uint64 va;
	uint32 vaild; //1->has been allocate
	block blocks[Max_block];
}page;

// the extremely simple definition of process, used for begining labs of PKE
typedef struct process {
  // pointing to the stack used in trap handling.
  uint64 kstack;
  // user page table
  pagetable_t pagetable;
  // trapframe storing the context of a (User mode) process.
  trapframe* trapframe;
  //manage pages
  page pages;
}process;

// switch to run user app
void switch_to(process*);

// current running process
extern process* current;
// virtual address of our simple heap
extern uint64 g_ufree_page;

#endif

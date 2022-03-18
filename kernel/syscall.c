/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  //buf is an address in user space on user stack,
  //so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ...
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va".
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}
uint64 sys_user_allocate_page_pro(uint64 size){
    if(size>PGSIZE)
    panic("size is bigger than the size of page!");
    if(current->pages.vaild==0) //未分配
    {
        page new_page;
        new_page.va = sys_user_allocate_page();
        new_page.vaild = 1;
        new_page.blocks[0].va = new_page.va;
        new_page.blocks[0].valid=1;
        new_page.blocks[0].size = size;
        current->pages = new_page;
        return new_page.va;
    }
    //已分配了page，遍历page的找到第一个未分配的block;
    int i=0;
    uint64 allocated_space = 0;
    for(i=0; i<Max_block; i++){
        if(i!=Max_block-1&&current->pages.blocks[i].valid==0)
        {
            break;
        }
        allocated_space += current->pages.blocks[i].size;
    }
    if(i==Max_block)
    panic("no enough block!\n");
    uint64 spare_space = PGSIZE - allocated_space;
    if(spare_space<size)
    panic("no enough space!\n");
    // 有空闲空间可以分配
    if(i!=0&&current->pages.blocks[0].va-current->pages.va>=size){
        //page的头部还有未分配的空间
        sprint("----------------\n");
        block new_block;
        new_block.va = current->pages.blocks[0].va - size;
        new_block.valid = 1;
        new_block.size = size;
        for(int k=Max_block-1; k>i; k--){
            current->pages.blocks[k] =  current->pages.blocks[k-1];
        }
        current->pages.blocks[0] = new_block;
        return new_block.va;
    }
    block pre_block = current->pages.blocks[i-1];
    block new_block;
    new_block.va = pre_block.size + pre_block.va;
    new_block.valid = 1;
    new_block.size = size;
    current->pages.blocks[i] = new_block;
    return new_block.va;
}
uint64 sys_user_free_page_pre(uint64 va, uint64 size)
{
    if(current->pages.vaild==0) //未分配
    panic("nothing to free!\n");
    int i=0;
    for(i=0; i<Max_block; i++){
        if(current->pages.blocks[i].valid==0)
        panic("there is no target block to free\n");
        if(current->pages.blocks[i].va==va)
        break;
    }
    if(i==Max_block)
    panic("find no va equal\n");
    current->pages.blocks[i].valid=0;
    int j;
    for(j = i; j<Max_block-1; j++)
    {
        if(current->pages.blocks[j+1].valid==0)
        {
            current->pages.blocks[j].valid=0;
            break;
        }
        current->pages.blocks[j] = current->pages.blocks[j+1];
    }
    if(j==0)
    {
        sys_user_free_page(current->pages.va); 
        current->pages.vaild=0;
        //页中的所有块都已经free了，这时page也可以进行free；
        return 0;
    }
    return 0;
}
//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_allocate_page:
    //   return sys_user_allocate_page();
        return sys_user_allocate_page_pro(a1);
    case SYS_user_free_page:
      return sys_user_free_page_pre(a1,a2);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}

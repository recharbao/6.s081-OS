#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64 sys_mmap(void)
{
  int length;
  int prot;
  int flags;
  int fd;
  int offset;
  struct proc *p = myproc();

  if(argint(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0) {
    return -1;
  }

  // printf("length out = %d\n", length);

  for(int i = 0; i < NVMA; i++) {
    if (p->vma[i].status == 0) {

      // printf("j = %d\n", i);

      if((p->ofile[fd]->readable && !p->ofile[fd]->writable) && (prot & PROT_WRITE) && (flags & MAP_SHARED)) {
        // printf("readable = %d\n", p->ofile[fd]->readable);
        // printf("writable = %d\n", p->ofile[fd]->writable);
        return -1;
      }
      p->vmastatue = 1;
      p->vma[i].f = p->ofile[fd];
      filedup(p->vma[i].f);
      p->vma[i].length = length;
      p->vma[i].permissions = prot;
      p->vma[i].flags = flags;
      p->vma[i].status = 1;
      p->vma[i].offset = offset;

      printf("length = %d\n", length);
      printf("p->ofile[fd].size = %d\n", p->ofile[fd]->ip->size);
      int sz = PGROUNDUP(p->ofile[fd]->ip->size);
    
      while(sz < length) {
      
        char* mem = kalloc();
        if (mem == 0) {
          return -1;
        }

        memset(mem, 0, PGSIZE);

        if (mappages(p->pagetable, p->vma[i].vma_start+sz+1, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0) {
          kfree(mem);
          return -1;
        }
        sz += PGSIZE;
      }

      printf("after  length = %d\n", length);
      printf("after   p->ofile[fd].size = %d\n", p->ofile[fd]->ip->size);

      return p->vma[i].vma_start;
    }
  }

  // printf("length = %d\n", length);
  return -1;
}

uint64 sys_munmap(void)
{
  uint64 address;
  int length;

  struct proc *p = myproc();

  if (argaddr(0, &address) < 0 || argint(1, &length) < 0){
    release(&p->lock);
    return -1;
  }

  for (int i = 0; i < NVMA; i++) {
    if (address >= p->vma[i].vma_start && address < p->vma[i].vma_end) {
      if (p->vma[i].flags == MAP_SHARED) {
        // printf("i = %d\n", i);
        filewrite(p->vma[i].f, p->vma[i].vma_start, p->vma[i].length);
      }
        // printf("after filewrite\n");
        // fileclose(p->vma[i].f);
        p->vma[i].status = 0;
        // printf("i = %d\n", i);
        // printf("address = %p\n", address);
        // printf("p->vma[i].vma_start = %p\n", p->vma[i].vma_start);
        // printf("vma length = %p\n", p->vma[i].vma_end - p->vma[i].vma_start);
        // printf("length = %p\n", length);
        // printf("p->vma[i].length = %p\n", p->vma[i].length);
        uvmunmap(p->pagetable, address, length, 1);
        return 0;
    }
    
  }
  
  return -1;
}

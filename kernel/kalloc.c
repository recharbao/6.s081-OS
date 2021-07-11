// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct {
  char* pa_start;
  // uint64 pa_end;
  struct spinlock lock;
  char  *ref_count;
}ref;

struct run {
  struct run *next;
  // int ref_count;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  ref.ref_count = (char*)p;
  ref.pa_start = (char*)(p + 16*PGSIZE);
  memset(ref.ref_count, 1, 16*PGSIZE);
  //printf("ref.count = %d\n", ref.ref_count[1]);
  // printf("ref.count = %u\n", (uint)ref.ref_count[1]);
  p = ref.pa_start;
  // ref.pa_end = (char*)PGROUNDDOWN((uint64)pa_end);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  // printf("freerange !\n");
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  // printf("kfree !\n");
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  
  acquire(&ref.lock);
  int index = ((char*)pa - ref.pa_start) / PGSIZE;
  ref.ref_count[index]--;
  release(&ref.lock);

  acquire(&kmem.lock);
  // printf("free   ref.ref_count = %d\n", ref.ref_count[index]);
  if(ref.ref_count[index] <= 0) {
    // printf("kfree if !\n");
    memset(pa, 1, PGSIZE);  // attention: cannot set before !
    r = (struct run*)pa;
    ref.ref_count[index] = 0;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  
  release(&kmem.lock);
}


// void
// kfree_init(void *pa)
// {
//   // printf("kfree_init here1 !\n");
//   struct run *r;

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("kfree");

//   // Fill with junk to catch dangling refs.
//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;

//   acquire(&kmem.lock);

//   // printf("kfree_init here2 !\n");

//   int index = ((char*)pa - ref.pa_start) / PGSIZE;
//   // printf("index = %d\n", index);
//   // printf("kfree_init here3 !\n");
//   ref.ref_count[index] = 0;
//   r->next = kmem.freelist;
//   kmem.freelist = r;
//   // printf("kfree_init here4 !\n");
//   release(&kmem.lock);
// }

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // printf("kalloc !\n");
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r) {
    // printf("kalloc if !\n");
     memset((char*)r, 5, PGSIZE); // fill with junk
     acquire(&ref.lock);
     int index = ((char*)r - ref.pa_start) / PGSIZE;
     ref.ref_count[index]++;
     release(&ref.lock);
  }
   
  // printf("kalloc last !\n");
  return (void*)r;
}


void 
incre_ref_count(void* pa) {
  // printf("incre_ref_count before!\n");
  struct run *r;
  r = (struct run*)pa;
  // acquire(&kmem.lock);
  acquire(&ref.lock);
  if(r) {
    int index = ((char*)pa - ref.pa_start) / PGSIZE;
    // printf("pa = %d\n", pa);
    // printf("ref.pa_start = %d\n", ref.pa_start);
    // printf("index = %d\n", index);
    // printf("ref_count = %d\n", ref.ref_count[index]);
    ref.ref_count[index]++;
  }
  release(&ref.lock);
  // release(&kmem.lock);
  // printf("incre_ref_count after!\n");
}
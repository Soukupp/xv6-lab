// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

int ref_count[PHYSTOP/PGSIZE];

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
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
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref_count[(uint64)p/PGSIZE]=1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  int index = (uint64)pa / PGSIZE;
  if(ref_count[index]<1)
    panic("kfree rf");
  ref_count[index]--;
  
  if(ref_count[index]>0){
    release(&kmem.lock);
    return;
  }
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

int
get_ref_count(void* pa){
  acquire(&kmem.lock);
  int index = (uint64)pa / PGSIZE;
  int tmp = ref_count[index];
  release(&kmem.lock);
  return tmp;
}

void
add_ref_count(void* pa){
  acquire(&kmem.lock);
  int index = (uint64)pa / PGSIZE;
  if((uint64)pa>=PHYSTOP||ref_count[index]<1)
    panic("incre");
  ref_count[index]++;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    int index = (uint64)r/PGSIZE;
    if(ref_count[index]!= 0){
      panic("kalloc");
    }
    ref_count[index]=1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

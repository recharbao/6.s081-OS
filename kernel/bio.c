// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock bucketLock[NBUCKET];

  struct buf bucketBuf[NBUCKET];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucketLock[i], "bcache bcacheLock");
    bcache.bucketBuf[i].prev = &bcache.bucketBuf[i];
    bcache.bucketBuf[i].next = &bcache.bucketBuf[i];
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    // b->refcnt = 0;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    b->status = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint64 devLeftMove = (uint64)dev << 32;
  int key = (devLeftMove | blockno) % NBUCKET;

  acquire(&bcache.bucketLock[key]);

  // printf("bio  here1 !\n");

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // if (&bcache.bucketBuf[key]) {
    
  // }else {
    
  // }
  
  for(b = bcache.bucketBuf[key].next; b != &bcache.bucketBuf[key]; b = b->next) {
    // printf("bio  here3 !\n");
    if (b->dev == dev && b->blockno == blockno && b->refcnt > 0) {
      __sync_fetch_and_add(&b->refcnt, 1);
      // b->refcnt++;
      release(&bcache.bucketLock[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // printf("bio  here2 !\n");
  

  // // Not cached; recycle an unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if(b->status == 0 && __sync_bool_compare_and_swap(&b->status, 0, 1)) {
    // if(b->refcnt == 0 && __sync_bool_compare_and_swap(&b->refcnt, 0, 1)) {
      b->refcnt = 1;
      b->valid = 0;
      b->blockno = blockno;
      b->dev = dev;
      b->next = bcache.bucketBuf[key].next;
      bcache.bucketBuf[key].next->prev = b;
      bcache.bucketBuf[key].next = b;
      b->prev = &bcache.bucketBuf[key];
      release(&bcache.bucketLock[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}


// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint64 devLeftMove = (uint64)b->dev << 32;
  int key = (devLeftMove | b->blockno) % NBUCKET;
  acquire(&bcache.bucketLock[key]);
  // b->refcnt--;
  __sync_fetch_and_sub(&b->refcnt, 1);

  if (b->refcnt == 0) {
    
    // printf("bio  here2 !\n");
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;

    // bcache.bucketBuf[key].next = bcache.bucketBuf[key].next->next;
    // bcache.bucketBuf[key].next->prev = &bcache.bucketBuf[key];

    // debug
    b->next->prev = b->prev;
    b->prev->next = b->next;

    // if(b->refcnt != 0)
    //   printf("b->ref = %d\n", b->refcnt);

    __sync_bool_compare_and_swap(&b->status, 1, 0);
  }
  
  release(&bcache.bucketLock[key]);
}

void
bpin(struct buf *b) {
  uint64 devLeftMove = (uint64)b->dev << 32;
  int key = (devLeftMove | b->blockno) % NBUCKET;
  acquire(&bcache.bucketLock[key]);
  __sync_fetch_and_add(&b->refcnt, 1);
  // b->refcnt++;
  release(&bcache.bucketLock[key]);
}

void
bunpin(struct buf *b) {
  uint64 devLeftMove = (uint64)b->dev << 32;
  int key = (devLeftMove | b->blockno) % NBUCKET;
  acquire(&bcache.bucketLock[key]);
  __sync_fetch_and_sub(&b->refcnt, 1);
  // b->refcnt--;
  release(&bcache.bucketLock[key]);
}



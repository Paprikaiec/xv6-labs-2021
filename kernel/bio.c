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

#define NHASH 13

struct {
  struct spinlock hashLock[NHASH];
  struct buf devBnoMap[NHASH];

  struct buf head;
  struct spinlock listLock;

  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

int devBno2hash(uint dev, uint blockno) {
  return (dev * FSSIZE + blockno) % NHASH;
}

// struct buf* hashGet(uint dev, uint blockno) {
//   int devBnoKey = devBno2hash(dev, blockno);
//   acquire(&bcache.hashLock[devBnoKey]);
//   struct buf* prev = &(bcache.devBnoMap[devBnoKey]);
//   while (prev->nextHash) {
//     struct buf* pos = prev->nextHash;
//     if (pos->dev == dev && pos->blockno == blockno) {
//       pos->refcnt ++;
//       release(&(bcache.hashLock[devBnoKey]));
//       acquiresleep(&(pos->lock));
//       return pos;
//     }
//     prev = prev->nextHash;
//   }
//   release(&bcache.hashLock[devBnoKey]);
//   return 0;
// }

void hashPut(struct buf* b) {
  int devBnoKey = devBno2hash(b->dev, b->blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  struct buf* prev = &(bcache.devBnoMap[devBnoKey]);
  while (prev->nextHash)
    prev = prev->nextHash;
  prev->nextHash = b; 
  release(&bcache.hashLock[devBnoKey]);
}

void hashRemove(struct buf* b) {
  int devBnoKey = devBno2hash(b->dev, b->blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  struct buf* prev = &(bcache.devBnoMap[devBnoKey]);
  while (prev->nextHash) {
    struct buf* pos = prev->nextHash;
    if (pos->dev == b->dev && pos->blockno == b->blockno) {
      prev->nextHash = pos->nextHash;
      pos->nextHash = 0;
      break;
    }
    prev = prev->nextHash;
  }
  release(&bcache.hashLock[devBnoKey]);
}


// // Put buf to head deque first.
// void listInsert(struct buf* b) {
//   // acquire(&bcache.listLock);
//   // printf("pos->refcnt%d\n", b->refcnt);
//   b->next = bcache.head.next;
//   b->prev = &bcache.head;
//   bcache.head.next->prev = b;
//   bcache.head.next = b;
//   // release(&bcache.listLock);
// }

// // Get head deque least used from last buf.
// struct buf* listStrip() {
//   struct buf* pos;
//   // acquire(&bcache.listLock);
//   // printf("begin\n%x", &bcache.head);
//   for(pos = bcache.head.prev; pos != &bcache.head; pos = pos->prev) {
//     // printf("->%x, %d", pos, pos->refcnt);
//     if (pos->refcnt == 0) {
//       if (pos->dev)
//         hashRemove(pos);
//       // pos->prev->next = pos->next;
//       // pos->next->prev = pos->prev;
//       return pos;
//       // printf("---------end-------\n");
//     }
//   }


//   for (pos = bcache.buf; pos < bcache.buf+NBUF; pos++) {
//     printf("%d\n", pos->refcnt);
//   }
//   panic("bget: no buffers");

//   // release(&bcache.listLock);
// }


void
binit(void)
{
  int len = 15;
  char str[len];

  for (int i = 0; i < NHASH; i++) {
    snprintf(str, len, "bcache_lock_%d", i);
    // initlock(&bcache.hashLock[i], str);
    bcache.devBnoMap[i].nextHash = 0;
  }

  struct buf *b;

  // initlock(&bcache.listLock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++) {
    b->dev = 0; // TODO: need to confirm dev will only be 1
    // b->ticks = 0;

    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
    initsleeplock(&b->lock, "buffer");
  }
  // printf("%p",&bcache.head);
  // for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
  //   printf("->%p", b);
  // }
  // panic("stop");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;


  // Is the block already cached?
  // if ((b = hashGet(dev, blockno))) 
  //   return b;
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  int devBnoKey = devBno2hash(dev, blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  b = bcache.devBnoMap[devBnoKey].nextHash;
  while (b) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt ++;
      release(&(bcache.hashLock[devBnoKey]));
      // acquire(&tickslock);
      // b->ticks = ticks;
      // release(&tickslock);
      acquiresleep(&(b->lock));
      return b;
    }
    b = b->nextHash;
  }
  release(&bcache.hashLock[devBnoKey]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.

  // acquire(&tickslock);
  // b = bcache.buf;
  // for (struct buf* temp = bcache.buf + 1; temp < bcache.buf + NBUF; temp++) {
  //   if (b->ticks < temp->ticks) {
  //     b = temp;
  //   }
  // }
  // b->ticks = ticks;
  // release(&tickslock);

  // // if (b->refcnt > 0)
  // //   panic("bget: no buffers");

  // hashRemove(b);
  // // b->refcnt = 1;
  // b->dev = dev;
  // b->blockno = blockno;
  // b->valid = 0;
  // hashPut(b);
  // acquiresleep(&b->lock);
  // return b; 

  acquire(&bcache.listLock);
  for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
    if (b->refcnt == 0) {
      hashRemove(b);
      b->refcnt = 1;
      release(&bcache.listLock);
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      hashPut(b);
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
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int devBnoKey = devBno2hash(b->dev, b->blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  b->refcnt--;
  release(&bcache.hashLock[devBnoKey]);
  if (b->refcnt == 0) {
    // Put buf to the head.next
    acquire(&bcache.listLock);
    b->prev->next = b->next;
    b->next->prev = b->prev;

    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
    release(&bcache.listLock);
  }
  
 
}

void
bpin(struct buf *b) {
  int devBnoKey = devBno2hash(b->dev, b->blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  b->refcnt++;
  release(&bcache.hashLock[devBnoKey]);
  // acquire(&tickslock);
  // b->ticks = ticks;
  // release(&tickslock);

}

void
bunpin(struct buf *b) {
  int devBnoKey = devBno2hash(b->dev, b->blockno);
  acquire(&bcache.hashLock[devBnoKey]);
  b->refcnt--;
  release(&bcache.hashLock[devBnoKey]);
}



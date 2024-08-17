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

#define HASH_NUMBER 73
#define INVALID_DEV 300000
struct {
  // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf *(head[HASH_NUMBER]);
  struct spinlock lockhash[HASH_NUMBER];
} bcache;
int bhash(uint dev, uint blockno) {
  return ((dev*dev+37)* (blockno%37) * (blockno%59) % HASH_NUMBER);
}
void
binit(void)
{
  struct buf *b;
  int i;

  push_off();
  if(cpuid()==0)
  {
  for(i=0;i<HASH_NUMBER;i++)
  {
    bcache.head[i]=0;
    initlock(&(bcache.lockhash[i]), "bcache");
  }
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.head[(b-bcache.buf)%HASH_NUMBER];
    b->refcnt=0;
    b->valid=0;
    b->dev=INVALID_DEV;
    bcache.head[(b-bcache.buf)%HASH_NUMBER] = b;
    initsleeplock(&b->lock, "buffer");
  }}
  pop_off();
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf * temp;
  int i = 0;
  int is_find = 0;
  const int hashno = bhash(dev, blockno);

  acquire(&(bcache.lockhash[hashno]));

  // Is the block already cached?
  for (b = bcache.head[hashno]; b != 0; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&(bcache.lockhash[hashno]));
      goto end;
    }
  }
  release(&(bcache.lockhash[hashno]));

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // printf("%d ",hashno);
  for (i = hashno; i < HASH_NUMBER; i++)
  {
    acquire(&(bcache.lockhash[i]));
    struct buf **prev = &(bcache.head[i]);
    for (b = bcache.head[i]; b != 0; b = b->next)
    {
      // if(b->dev!=INVALID_DEV&&bhash(b->dev,b->blockno)!=i)
      // {
      //   panic("bget: hash not match");
      // }
      if (b->refcnt == 0)
      {
        *prev = b->next;
        release(&(bcache.lockhash[i]));
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        temp=b;
        acquire(&(bcache.lockhash[hashno]));
        for (b = bcache.head[hashno]; b != 0; b = b->next)
        {
          if (b->dev == dev && b->blockno == blockno)
          {
            b->refcnt++;
            is_find = 1;
            break;
          }
        }
        temp->next = bcache.head[hashno];
        bcache.head[hashno] = temp;
        if(is_find)
        {
          temp->refcnt=0;
          temp->dev=INVALID_DEV;
        }else
        {
          b=temp;
        }
        release(&(bcache.lockhash[hashno]));
        goto end;
      }
      prev = &(b->next);
    }

    release(&(bcache.lockhash[i]));
  }
  for (i = 0; i < hashno; i++)
  {
    acquire(&(bcache.lockhash[i]));
    struct buf **prev = &(bcache.head[i]);
    for (b = bcache.head[i]; b != 0; b = b->next)
    {
      // if(bhash(b->dev,b->blockno)!=i&&b->dev!=INVALID_DEV)
      // {
      //   panic("bget: hash not match");
      // }
      if (b->refcnt == 0)
      {
        *prev = b->next;
        release(&(bcache.lockhash[i]));
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        temp=b;
        acquire(&(bcache.lockhash[hashno]));
        for (b = bcache.head[hashno]; b != 0; b = b->next)
        {
          if (b->dev == dev && b->blockno == blockno)
          {
            b->refcnt++;
            is_find = 1;
            break;
          }
        }
        temp->next = bcache.head[hashno];
        bcache.head[hashno] = temp;
        if(is_find)
        {
          temp->refcnt=0;
          temp->dev=INVALID_DEV;
        }else
        {
          b=temp;
        }
        release(&(bcache.lockhash[hashno]));
        goto end;
      }
      prev = &(b->next);
    }
    release(&(bcache.lockhash[i]));
  }
  panic("bget: no buffers");

end:
if(b->dev!=dev||b->blockno!=blockno)
{
  panic("bget: dev or blockno not match");

}
  acquiresleep(&b->lock);
  return b;
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
  if(b->refcnt==0&&b->valid)
  {
    panic("bwrite: refcnt is zero");
  }
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
  const int hashno=bhash(b->dev,b->blockno);
  if(b->dev==INVALID_DEV)
  {
    panic("brelse: invalid dev");
  }
  releasesleep(&b->lock);

  acquire(&(bcache.lockhash[hashno]));
  // __sync_sub_and_fetch(&b->refcnt,1);
  b->refcnt--;
  release(&(bcache.lockhash[hashno]));
}

void
bpin(struct buf *b) {
  const int hashno=bhash(b->dev,b->blockno);
  acquire(&(bcache.lockhash[hashno]));
  b->refcnt++;
  release(&(bcache.lockhash[hashno]));
}

void
bunpin(struct buf *b) {
  const int hashno=bhash(b->dev,b->blockno);
  acquire(&(bcache.lockhash[hashno]));
  b->refcnt--;
  release(&(bcache.lockhash[hashno]));
}



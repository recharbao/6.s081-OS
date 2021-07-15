//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}


//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

void close_socket(struct sock *so)
{
  // printf("close_socket ! \n");
  // acquire(&lock);
  struct sock *s = sockets;
  if (s->next == 0) {
    sockets = 0;
  }
  while(s && s->next != so) {
    s = s->next;
    // printf("sadas\n");
  }
  if(s)
    s->next = so->next;
  // release(&lock);
  kfree(so);
  // printf("close_socket end !\n");
}


int write_socket(struct sock *so, uint64 addr, int n)
{

  // printf("write_socket here 0 !\n");

  struct proc *pr = myproc();

  acquire(&so->lock);
  int headeroom = sizeof(struct udp) + sizeof(struct ip) + sizeof(struct eth);
  struct mbuf *b = mbufalloc(headeroom);

  // printf("addr = %p\n", addr);
  
  // uint len = (uint)strlen((char*)addr);
  // printf("write_socket here 2 !\n");
  mbufput(b, n);

  if(copyin(pr->pagetable, b->head, addr, b->len) == -1) {
    release(&so->lock);
    mbuffree(b);
    return -1;
  }
  
  net_tx_udp(b, so->raddr, so->lport, so->rport);
  release(&so->lock);
  

  return n;
}

int read_socket(struct sock *so, uint64 addr, int n)
{

  // printf("read_socket !\n");
  struct proc *pr = myproc();

  acquire(&so->lock);
  while(mbufq_empty(&so->rxq)) {
    // printf("read_socket empty !\n");
    if (pr->killed) {
      release(&so->lock);
      return -1;
    }
    sleep(&so->lport, &so->lock);
  }

  // printf("read_socket here !\n");

  struct mbuf *b = mbufq_pophead(&so->rxq);
  // int len = strlen(b->head)+1;
  int len = b->len;       // There is a difference between the two, first may be lost 0
  if(copyout(pr->pagetable, addr, b->head, len) == -1) {
      release(&so->lock);
      mbuffree(b);
      return -1;
  }
  // wakeup(&so->rport);
  release(&so->lock);

  // b->len -= (n < b->len ? n : b->len);
  // if (b->len == 0){
  mbuffree(b);
  // }

  // printf("b->len = %d\n", b->len);
  // printf("read_socket (n < b->len ? n : b->len) = %d\n", (n < b->len ? n : b->len));
  // printf("strlen(b->head)+1 = %d\n", len);
  return len;
}


// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  // printf("sockrecvudp !\n");
  struct sock *first = sockets;
  while(first) {
    if(first->raddr == raddr && first->lport == lport && first->rport == rport) {
      // wakeup(&first->lport);
      break;
    }
    first = first->next;
  }

  if(!first)
    mbuffree(m);
  mbufq_pushtail(&first->rxq, m);
  // __sync_synchronize();

  wakeup(&first->lport);
}

//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "fcntl.h"

#define BUFSIZE 512

extern struct inode *safecreate(char *path, short type, short major, short minor);
extern struct report *data;

struct devsw devsw[NDEV];
struct
{
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file *
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for (f = ftable.file; f < ftable.file + NFILE; f++)
  {
    if (f->ref == 0)
    {
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file *
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if (f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if (f->ref < 1)
    panic("fileclose");
  if (--f->ref > 0)
  {
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if (ff.type == FD_PIPE)
  {
    pipeclose(ff.pipe, ff.writable);
  }
  else if (ff.type == FD_INODE || ff.type == FD_DEVICE)
  {
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;

  if (f->type == FD_INODE || f->type == FD_DEVICE)
  {
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

int readf(struct file *f, int user_src, uint64 addr, int n){
  int r = 0;
  ilock(f->ip);
  if ((r = readi(f->ip, user_src, addr, f->off, n)) > 0)
    f->off += r;
  iunlock(f->ip);
  return r;
}

// Read from file f.
// addr is a user virtual address.
int fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if (f->readable == 0)
    return -1;

  if (f->type == FD_PIPE)
  {
    r = piperead(f->pipe, addr, n);
  }
  else if (f->type == FD_DEVICE)
  {
    if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  }
  else if (f->type == FD_INODE){  
    r = readf(f, 1, addr, n);
  }
  else
  {
    panic("fileread");
  }

  return r;
}

int writef(struct file *f, int user_src, uint64 addr, int n)
{
  int r, ret = 0;
  // write a few blocks at a time to avoid exceeding
  // the maximum log transaction size, including
  // i-node, indirect block, allocation blocks,
  // and 2 blocks of slop for non-aligned writes.
  // this really belongs lower down, since writei()
  // might be writing a device like the console.
  int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;
  int i = 0;
  while (i < n)
  {
    int n1 = n - i;
    if (n1 > max)
      n1 = max;
    ilock(f->ip);
    if ((r = writei(f->ip, user_src, addr + i, f->off, n1)) > 0)
      f->off += r;
    iunlock(f->ip);
    if (r != n1)
    {
      // error from writei
      break;
    }
    i += r;
  }
  ret = (i == n ? n : -1);
  return ret;
}

int filereadend(char *filename, void *data, int struct_size){
  begin_op();
  struct file *f;
  struct inode *ip;
  int n;

  ip = namei(filename);
  if (ip == 0){
    printf("\nkernel_read_end: no such file %s\n", filename);
  end_op();
    return 0;
  }

  f = filealloc();
  if (f == 0)
  {
    printf("\nkernel_append_struct: failed to allocate file\n");
  end_op();
    return -1;
  }

  f->ip = ip;
  int fileSize = (int)f->ip->size;

  if(fileSize - struct_size > 0){
    f->off = f->ip->size - (uint)struct_size;
  }else{
    f->off = 0;
  }
  f->readable = 1;
  f->writable = 0;
  f->type = FD_INODE;

  int readsize = fileSize - f->off;

  printf("before read\nreadsize: %d\n%d\n", readsize,f->ip->size);
  if((n = readf(f, 0, (uint64)data, readsize)) != readsize){
    printf("\nkernel_read_struct: failed to read struct from file\n");
    fileclose(f);
    return -1;
  }
  printf("\nafter read\n");
  end_op();
  printf("\nafter_endop\n");

  return readsize;
}

// Kernel function to append a struct to a file
int fileappend(char *filename, void *data, int struct_size)
{
  begin_op();
  struct file *f;
  struct inode *ip;
  int n;
  // Open the file in read-write mode (kernel-level)
  ip = namei(filename);
  printf("\nafter namei\n");
  if (ip == 0)
  {
    // If the file doesn't exist, create it
    ip = safecreate(filename, T_FILE, 0, 0);
    if (ip == 0)
    {
      printf("\nkernel_append_struct: failed to create file %s\n", filename);
  end_op();
      return -1;
    }
  }
  printf("\nbefore filealloc\n");
  f = filealloc();
  if (f == 0)
  {
    printf("\nkernel_append_struct: failed to allocate file\n");
  end_op();
    return -1;
  }

  f->ip = ip;
  f->off = f->ip->size; // Start at the end
  f->readable = 1;
  f->writable = 1;
  f->type = FD_INODE;
  printf("\nbefore write\n");

  // Write the struct data to the file at the end
  if ((n = writef(f, 0, (uint64)data, struct_size)) != struct_size)
  {
    printf("\nkernel_append_struct: failed to write struct to file\n");
    fileclose(f);
    return -1;
  }
  printf("\nafter write\n");
  end_op();
  printf("\nafter_endop\n");

  return 0;
}

// Write to file f.
// addr is a user virtual address.
int filewrite(struct file *f, uint64 addr, int n)
{
  int ret = 0;

  if (f->writable == 0)
    return -1;

  if (f->type == FD_PIPE)
  {
    ret = pipewrite(f->pipe, addr, n);
  }
  else if (f->type == FD_DEVICE)
  {
    if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  }
  else if (f->type == FD_INODE)
  {
    ret = writef(f, 1, addr, n);
  }
  else
  {
    panic("filewrite");
  }

  return ret;
}

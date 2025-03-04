/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

char *error_msgs[] = {
    "Success",                  // 0
    "Operation not permitted",  // EPERM 1
    "No such file or directory", // ENOENT 2
    "No such process",          // ESRCH 3
    "Interrupted system call",  // EINTR 4
    "I/O error",                // EIO 5
    "No such device or address", // ENXIO 6
    "Argument list too long",   // E2BIG 7
    "Exec format error",        // ENOEXEC 8
    "Bad file number",          // EBADF 9
    "No child processes",       // ECHILD 10
    "Try again",                // EAGAIN 11
    "Out of memory",            // ENOMEM 12
    "Permission denied",        // EACCES 13
    "Bad address",              // EFAULT 14
    "Unknown error"             // default
};

#define MAX_ERROR_INDEX 14

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror() {
    char buffer[256];
    char *msg;
    
    if (errno >= 0 && errno <= MAX_ERROR_INDEX) {
        msg = error_msgs[errno];
    } else {
        msg = error_msgs[MAX_ERROR_INDEX];
    }
    
    buffer[0] = 'E';
    buffer[1] = 'r';
    buffer[2] = 'r';
    buffer[3] = 'o';
    buffer[4] = 'r';
    buffer[5] = ' ';
    buffer[6] = '(';
    
    char errno_str[10];
    itoa(errno, errno_str);
    
    int i = 7;
    int j = 0;
    while (errno_str[j] != 0) {
        buffer[i++] = errno_str[j++];
    }
    
    buffer[i++] = ')';
    buffer[i++] = ':';
    buffer[i++] = ' ';
    
    j = 0;
    while (msg[j] != 0) {
        buffer[i++] = msg[j++];
    }
    
    buffer[i++] = '\n';
    buffer[i] = 0;
    
    write(1, buffer, i);
}
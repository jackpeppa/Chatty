//alcune macro utili
#if !defined(SYS_H_)
#define SYS_H_
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>

#define IFERROR(s,m) if((s)==-1) {perror(m); exit(errno);}
#define IFERROR2(s,m,c) if((s)==-1) {perror(m); c;}
#define CHECKNULL(s,m,c) if((s)==NULL) {perror(m); c;}
#define WRITE(m) IFERROR(write(STDOUT,m,strlen(m)), m);
#define WRITELN(m) WRITE(m);WRITE("\n");
#define MALLOC(ptr, sz)			   \
    if ((ptr=malloc(sz)) == NULL) { \
	perror("malloc");		   \
	exit(EXIT_FAILURE);		   \
    }
#define REALLOC(ptr,sz)    \
    if((ptr=realloc(ptr,sz)) == NULL) {  \
      perror("realloc");                 \
      exit(EXIT_FAILURE);                \
    }
#define STDIN 0
#define STDOUT 1
#define STDERR 2


#endif

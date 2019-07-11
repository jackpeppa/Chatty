//file richiesto per passare il testleaks


/* esegui.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
//fa la fork ed esegue saluta.c con exec, poi controlla lo stato con cui termina il figlio
int main(int argc, char *argv[])
{
   int pid,status;
   char *newargv[] = { "hello", "world", NULL };
   char *newenviron[] = { NULL };  //stringhe del tipo "nome=valore"
   pid=fork();
   if(pid==0)
   {
     printf("FIGLIO:\n");
     execve("./saluta", newargv, newenviron);
     printf("exec fallita\n");
     exit(1);
   }
   else if(pid>0)
   {
     pid=wait(&status);
     printf("PADRE:");
     if(WIFEXITED(status))
        printf("Terminavione volontaria di %d con stato %d\n",pid,WEXITSTATUS(status));
     else if(WIFSIGNALED(status))
        printf("terminazione involontaria per segnale %d\n",WTERMSIG(status));
   }
   else printf("fork fallita\n");
   exit(0);
}

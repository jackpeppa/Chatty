


/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  originale dell'autore
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include "lib_utils.h"
#include "threads.h"
#include "connections.h"


static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

//maschera segnali
sigset_t signal_mask;

//struttura di configurazione server
configurazione* conf;

int main(int argc, char *argv[])
{

    //setto la maschera
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    sigaddset (&signal_mask, SIGPIPE);
    sigaddset (&signal_mask, SIGTERM);
    sigaddset (&signal_mask, SIGQUIT);
    sigaddset (&signal_mask, SIGUSR1);

    IFERROR2(pthread_sigmask (SIG_BLOCK, &signal_mask, NULL),"sigmask() error",exit(EXIT_FAILURE));

    //creo thread per gestione segnali
    pthread_t  sig_thr_id;
    IFERROR2(pthread_create (&sig_thr_id, NULL, signal_thread,NULL),"pthread_create error",exit(EXIT_FAILURE));

    //configuro il server
    MALLOC(conf,sizeof(configurazione));
    memset(conf->UnixPath, 0, UNIX_PATH_MAX);
    memset(conf->DirName, 0, UNIX_PATH_MAX);
    memset(conf->StatFileName, 0, UNIX_PATH_MAX);
    if (argc<1)
    {
      usage(argv[0]);
      exit(0);
    }
    //parso il file di configurazione
    parse((char*)argv[2],conf);


    //avvio il threadpool
    server_start();

    free(conf);
    pthread_join(sig_thr_id,NULL);
    return 0;
}

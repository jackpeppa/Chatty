/**
 * @file threads.h
 * @brief file per la gestione dei thread (listener,worker,signal thread)
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#define _POSIX_C_SOURCE 200809L
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<errno.h>
#include<signal.h>
#include<pthread.h>
#include"queue.h"
#include<sys/select.h>
#include<dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include"sysmacro.h"
#include"lib_utils.h"
#include"ops.h"
#include<connections.h>
#include"message.h"
#include"stats.h"


/**
* @brief argomenti threads
* @var thid thread id
*/
typedef struct
{
  int      thid;
} threadArgs_t;


/**
* @function thread_listener
* @brief thread che gestisce le connessioni
* @var arg argomento thread, in questo caso una struct threadArgs_t
*/
void* thread_listener(void* arg);

/**
* @function thread_worker
* @brief thread del pool
* @var arg argomento thread, in questo caso una struct threadArgs_t
*/
void* thread_worker(void* arg);

/**
* @function server_start
* @brief avvia il Threadpool di chatty
*/
void server_start();

/**
* @function signal_thread
* @brief thread che gestisce i segnali
*
* @param arg maschera dei segnali
*/
void* signal_thread(void* arg);

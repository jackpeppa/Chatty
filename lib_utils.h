/**
 * @file lib_utils.h
 * @brief file contenente parser e funzioni per la gestione di utenti/gruppi
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#if !defined(UTILS_H_)
#define UTILS_H_
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sysmacro.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include "stats.h"
#include "config.h"
#include "message.h"
#include "connections.h"
#include "icl_hash.h"

//caratteri da leggere per il parsing
#define LN 300


/**
* @struct configurazione
* @brief parammetri di avvio del server
* @var ovvie
*/
typedef struct
{
  char UnixPath[UNIX_PATH_MAX];
  unsigned int MaxConnections;
  unsigned int ThreadsInPool;
  unsigned int MaxMsgSize;
  unsigned int MaxFileSize;
  unsigned int MaxHistMsgs;
  char DirName[UNIX_PATH_MAX];
  char StatFileName[UNIX_PATH_MAX];
}configurazione;

/** utente chatty
* @var nome nome utente
* @var prvs array di messaggi history
* @var fd descrittore connessione
* @var online per capire se è online
* @var used per capire de la struct è utilizzata
* @var nprvs numero messaggi nella history
* @var oldest indice messaggio piu vecchio nella history
*/
typedef struct
{
  char nome[MAX_NAME_LENGTH+1];
  int fd;
  int online;
  int used;
  message_t* prvs;
  int nprvs;
  int oldest;
}utente;

/* gruppo
* @var nome nome gruppo
* @var creatore creatore gruppo
* @var persone componenti gruppo
*/
typedef struct
{
  char nome[MAX_NAME_LENGTH+1];
  char creatore[MAX_NAME_LENGTH+1];
  char* persone[MAX_GROUP_DIM];
}group;

/**
* @function parse
* @brief riempie una struct configurazione per l'avvio del server

* @param fname nome file di conf
* @param conf puntatore a struct configurazione
*/
void parse(char* fname, void* conf);


/**
* @function compara_utenti
* @brief vede se u1 e u2 sono lo stesso utente
* @return 1 se sono uguali,0 altrimenti
*/
int compara_utenti(void* u1,void* u2);
/**
* @function reg_op
* @brief registra l'utente
* @var fd descrittore connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int reg_op(int fd,message_t* msg);
/**
* @function conn_op
* @brief connette l'utente
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int conn_op(int fd,message_t* msg);
/**
* @function disc_op  (non trovata nel client ma richiesta)
* @brief connette l'utente
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int disc_op(int fd,message_t* msg);
/**
* @function unreg_op
* @brief deregistra l'utente
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int unreg_op(int fd,message_t* msg);
/**
* @function list_op
* @brief gestisce la richiesta USRLIST_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int list_op(int fd,message_t* msg);
/**
* @function txt_op
* @brief gestisce la richiesta POSTTXT_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int txt_op(int fd,message_t* msg);
/**
* @function prvs_op
* @brief gestisce la richiesta GETPREVMSGS_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 msg nella history -1 insuccesso 1 msg inviato
*/
int prvs_op(int fd,message_t* msg);
/**
* @function txtall_op
* @brief gestisce la richiesta POSTTXTALL_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret -1 insuccesso , n con n=numero messaggi inviati in caso di successo
*/
int txtall_op(int fd,message_t* msg);
/**
* @function file_op
* @brief gestisce la richiesta POSTFILE_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int file_op(int fd,message_t* msg);
/**
* @function get_op
* @brief gestisce la richiesta  GETFILE_OP
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int get_op(int fd,message_t* msg);
/**
* @function create
* @brief gestisce la richiesta  creategroup_op
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int create(message_t* msg,int fd);
/**
* @function delete
* @brief gestisce la richiesta  deletegroup_op
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int delete(message_t* msg,int fd);
/**
* @function add
* @brief gestisce la richiesta  addgroup_op
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso
*/
int add(message_t* msg,int fd);
/**
* @function group_txt_op
* @brief gestisce l' invio di un messaggio in un gruppo
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso 1 se il destinatario non è groupname
*/
int group_txt_op(message_t* msg,int fd);
/**
* @function group_file_op
* @brief gestisce l' invio di un file in un gruppo
* @var fd descr connessione
* @var msg messaggio con le varie info
* @ret 0 successo -1 insuccesso 1 se il destinatario non è groupname
*/
//int group_file_op(message_t* msg,int fd);
/**
* @function init_utenti
* @brief inizializza strutture per la gestione utenti e gruppi
*/
void init_utenti(void);
/**
* @function delete_utenti
* @brief dealloca le strutture per la gestione utenti e gruppi
*/
void delete_utenti(void);





#endif

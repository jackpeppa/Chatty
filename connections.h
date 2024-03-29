/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#include<sys/socket.h>
#include<sys/un.h>
#include<errno.h>
#include<stdio.h>

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <message.h>
#include "sysmacro.h"
#include "config.h"

/**
 * @file  connection.h
 * @brief Contiene le funzioni che implementano il protocollo
 *        tra i clients ed il server
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

// -------- server side -----
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int readHeader(int connfd, message_hdr_t *hdr);
/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int readData(long fd, message_data_t *data);

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int readMsg(long fd, message_t *msg);

/**
 * @function sendAck
 * @brief spedisce l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore del messaggio da spedire
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int sendAck(int fd, message_t *msg);

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int sendRequest(long fd, message_t *msg);

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return 1 in caso di successo -1 in caso di errore
 */
int sendData(long fd, message_data_t *msg);


/* da completare da parte dello studente con eventuali altri metodi di interfaccia */


#endif /* CONNECTIONS_H_ */

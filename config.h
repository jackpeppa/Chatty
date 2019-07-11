/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32
#define MAX_LEN_LENGTH                  5
#define UNIX_PATH_MAX                    64

/* aggiungere altre define qui */

#define MAX_GROUP_DIM                 100     //numero max di utenti in un gruppo



// to avoid warnings like "ISO C forbids an empty translation unit"x
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */

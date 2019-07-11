/**
 * @file stats.h
 * @brief interfaccia per l'aggiornamento delle statistiche
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>
#include <sysmacro.h>
#include <pthread.h>

struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
};


/**
*  @function funzioni per le statistiche
*  @brief aggiungono l'intero el al campo di chattystats per aggiornarle
*/
void nusers(int el);
void nonline(int el);
void ndelivered(int el);
void nnotdelivered(int el);
void nfiledelivered(int el);
void nfilenotdelivered(int el);
void nerrors(int el);

/**
*  @function stat_del
*  @brief elimina il mutex delle statistiche
*/
void stat_del(void);

/**
* @function get_nonline
* @brief restiruisce il numero di utenti online
*/
int get_nonline();
/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento
 */
static inline int printStats(FILE *fout) {
    extern struct statistics chattyStats;

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers,
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */

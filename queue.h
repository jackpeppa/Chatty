//coda concorrente utilizzata per il thread listener
#ifndef QUEUE_H_
#define QUEUE_H_

/** Elemento della coda.
 *
 */
typedef struct Node {
    void        * data;
    struct Node * next;
} Node_t;

/** Struttura dati coda.
 *
 */
typedef struct Queue {
    Node_t        *head;
    Node_t        *tail;
    unsigned long  qlen;
} Queue_t;


/** Alloca ed inizializza una coda. Deve essere chiamata da un solo
 *  thread (tipicamente il thread main).
 *   \param nrow numero righe
 *   \param numero colonne
 *
 *   \retval NULL se si sono verificati problemi nell'allocazione (errno settato)
 *   \retval q puntatore alla coda allocata
 */
Queue_t *initQueue();

/** Cancella una coda allocata con initQueue. Deve essere chiamata da
 *  da un solo thread (tipicamente il thread main).
 *
 *   \param q puntatore alla coda da cancellare
 */
void deleteQueue(Queue_t *q);

/** Inserisce un dato nella coda.
 *   \param data puntatore al dato da inserire
 *
 *   \retval 0 se successo
 *   \retval -1 se errore (errno settato opportunamente)
 */
int    push(Queue_t *q, void *data);

/** guarda se la coda contiene un Elemento
* param data elemento da confrontare
* param compare funzione per comparare
* retval 0 successo
* retval -1 insuccesso
*/
int contains(Queue_t *q,void* data,int (*compare)(void*,void*));

/** Estrae un dato dalla coda.
 *
 *  \retval data puntatore al dato estratto.
 */
void  *pop(Queue_t *q);


unsigned long length(Queue_t *q);

#endif /* QUEUE_H_ */

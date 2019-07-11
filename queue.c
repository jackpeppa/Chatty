//coda concorrente utilizzata nel thread listener
#include <stdlib.h>
#include<stdio.h>
#include <assert.h>
#include <pthread.h>
#include <queue.h>

/**
 * @file queue.c
 * @brief File di implementazione dell'interfaccia per la coda
 */



static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  qcond = PTHREAD_COND_INITIALIZER;


/* ------------------- funzioni di utilita' -------------------- */

static Node_t *allocNode()         { return malloc(sizeof(Node_t));  }
static Queue_t *allocQueue()       { return malloc(sizeof(Queue_t)); }
static void freeNode(Node_t *node) { free((void*)node); }
static void LockQueue()            { pthread_mutex_lock(&qlock);   }
static void UnlockQueue()          { pthread_mutex_unlock(&qlock); }
static void UnlockQueueAndWait()   { pthread_cond_wait(&qcond, &qlock); }
static void UnlockQueueAndSignal() {
    pthread_cond_signal(&qcond);
    pthread_mutex_unlock(&qlock);
}

/* ------------------- interfaccia della coda ------------------ */

Queue_t *initQueue()
{
    Queue_t *q = allocQueue();
    if (!q) return NULL;
    q->head = allocNode();
    if (!q->head) return NULL;
    q->head->data = NULL;
    q->head->next = NULL;
    q->tail = q->head;
    q->qlen = 0;
    return q;
}

void deleteQueue(Queue_t *q)
{
    while(q->head != q->tail)
    {
      	Node_t *p = (Node_t*)q->head;
      	q->head = q->head->next;
      	freeNode(p);
    }
    if (q->head) freeNode((void*)q->head);
    free(q);
}

int push(Queue_t *q, void *data)
{
    Node_t *n = allocNode();
    n->data = data;
    n->next = NULL;
    LockQueue();
    q->tail->next = n;
    q->tail       = n;
    q->qlen      += 1;
    UnlockQueueAndSignal();
    //printf("inserito in coda %d  lunghezza coda : %d\n",*(int*)data,q->qlen);
    return 0;
}

int contains(Queue_t *q,void* data,int (*compare)(void*,void*))
{
  Node_t* nodo;
  LockQueue();
  int n= q->qlen;
  nodo=q->head;
  while(n!=0)
  {
    nodo=nodo->next;
    n--;
    if(compare((void*)data,(void*)nodo->data)) return 0;
  }
  UnlockQueueAndSignal();
  return -1;
}

void *pop(Queue_t *q)
{
    LockQueue();
    while(q->head == q->tail)
    {
	     UnlockQueueAndWait();
    }
    // locked
    assert(q->head->next);
    Node_t *n  = (Node_t *)q->head;
    void *data = (q->head->next)->data;
    q->head    = q->head->next;
    q->qlen   -= 1;
    assert(q->qlen>=0);
    UnlockQueue();
    freeNode(n);
    //printf("tolto dalla coda %d  lunghezza coda : %d\n",*(int*)data,q->qlen);
    return data;
}

// accesso in sola lettura non in mutua esclusione
unsigned long length(Queue_t *q) {
    LockQueue();
    unsigned long len = q->qlen;
    UnlockQueueAndSignal();
    return len;
}

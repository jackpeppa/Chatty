/**
 * @file stats.c
 * @brief implementazione stats.h
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#include"stats.h"
//mutex per accedere alla struct statistics
pthread_mutex_t stat_mtx = PTHREAD_MUTEX_INITIALIZER;
//struct di statistiche
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

void nusers(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nusers+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void nonline(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nonline+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

int get_nonline()
{
  int temp;
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  temp=chattyStats.nonline;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
  return temp;
}

void ndelivered(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.ndelivered+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void nnotdelivered(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nnotdelivered+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void nfiledelivered(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nfiledelivered+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void nfilenotdelivered(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nfilenotdelivered+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void nerrors(int el)
{
  IFERROR(pthread_mutex_lock(&stat_mtx),"lock stats");
  chattyStats.nerrors+=el;
  IFERROR(pthread_mutex_unlock(&stat_mtx),"unlock stats");
}

void stat_del(void)
{
  IFERROR(pthread_mutex_destroy(&stat_mtx),"destroy stats");
}

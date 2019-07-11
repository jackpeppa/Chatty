/**
 * @file threads.h
 * @brief file di implementazione threads.h
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#include"threads.h"
//struttura di configurazione
extern configurazione* conf;

//coda task(dal listener ai workers)
Queue_t* queue;

//coda per la select(in uscita dai workers verso il listener)
Queue_t* s_queue;

//pipe per aggiornare la select: leggo 0: aggiorno maschera, leggo 1: termino il threadpool
int fd_select[2];
//nome socket
char* sockname;

void* thread_listener(void* arg)
{
  int fd_sk;     //socket di connessione
  int fd_c;      //socket di I/O con i client
  int fd_num=0;  //max fd attivo
  int fd_NUM=1;  //sempre uguale a fd_num+1 (fd del socket)
  int fd;        //indice per verificare risultati select
  fd_set set;    //maschera che serve per aggiornare
  fd_set rdset;  //maschera che viene aggiornata ogni volta
  int nfdselected; // numero sescrittori restituiti dalla select
  char buf[1];  //buffer aggiornamento select
  int mythid = ((threadArgs_t*)arg)->thid; //thread id
  //descs[i] è 1 se quel descrittore è di un socket(serve per impedire che la select prenda descrittori di file)
  int descs[conf->MaxConnections*10];
  memset(descs,0,conf->MaxConnections*10);

  //struct indirizzo
  struct sockaddr_un sa;
  strncpy(sa.sun_path,sockname,UNIX_PATH_MAX);
  sa.sun_family=AF_UNIX;

  IFERROR2(pipe(fd_select),"pipe select failed\n",exit(EXIT_FAILURE));  //creo pipe di aggiornamento

  //impostazioni iniziali per il socket
  IFERROR2((fd_sk=socket(AF_UNIX,SOCK_STREAM,0)),"socket() failed\n",exit(EXIT_FAILURE));
  IFERROR2((bind(fd_sk,(struct sockaddr*)&sa,sizeof(sa))),"bind() failed\n",exit(EXIT_FAILURE));
  IFERROR2((listen(fd_sk,SOMAXCONN)),"listen() failed\n",exit(EXIT_FAILURE));

  //fd_num serve essere il max fd attivo
  if(fd_sk > fd_num) fd_num=fd_sk;
  if(fd_select[0] > fd_num) fd_num=fd_select[0];
  if(fd_select[1] > fd_num) fd_num=fd_select[1];


  //setto la tabella dei fd intercettabili
  FD_ZERO(&set);;      //azzero
  FD_SET(fd_select[0],&set);  //aggiungo descrittore di aggiornamento
  FD_SET(fd_sk, &set); //aggiungo descrittore del server

  fd_NUM=fd_num+3; //socket+ quelli della pipe

  fprintf(stdout,"Server primario %d : START con fd %d\n",getpid(),fd_sk);
  rdset=set;  //inizializzo
  while(1)
  {
    descs[fd_select[0]]=1;
    descs[fd_sk]=1;
    FD_SET(fd_select[0],&set);
    FD_SET(fd_sk, &set);
    //va ogni volta ripristinato perchè la select lo cambia
    rdset=set;
    FD_ZERO(&set);;

    //rimetto descrittori in uscita dai worker nella select
    int z=length(s_queue);
    while( z > 0){
      z--;
      fd_NUM++;
      int* c=((int*)pop(s_queue));
      descs[*c]=1;
      FD_SET(*c,&rdset);
      free(c);
    }



    //la select rimane in attesa di connessioni
    IFERROR2(nfdselected=select(fd_NUM,&rdset,NULL,NULL,NULL),"select() failed\n",exit(errno));
    //printf("select sbloccata!\n");

    if(nfdselected==1 && FD_ISSET(fd_select[0],&rdset))  //se leggo 0 devo aggiornare la maschera, 1 devo terminare il threadpool
    {
        IFERROR(read(fd_select[0],buf,1),"read fd_select");
        if(buf[0]=='1')   //devo terminare i threads
        {
          for(int i=0;i<conf->ThreadsInPool;i++)
          {
             int* t = malloc(sizeof(int));
             *t= -1;
             IFERROR(push(queue,t),"push() failed\n");
          }
          fprintf(stdout,"Thread terminato #%d\n",mythid);
          close(fd_sk);
          pthread_exit(NULL);
        }
        for(fd=3;fd<=fd_num;fd++) //aggiorno maschera
        {
          if(descs[fd]==1)
             FD_SET(fd,&set);
        }
        continue;
    }
    //la select ha infividuato una connessione
    //con il for iteriamo il numero massimo di descrittori per cercare quello che 'chiama'
    for(fd=3;fd<=fd_num;fd++)
    {
        if(FD_ISSET(fd,&rdset))
        {
          //se è il descrittore del server proviamo ad accettare una nuova connessione
          if(fd == fd_sk)
          {
            IFERROR2(fd_c = accept(fd_sk,NULL,NULL),"accept() failed\n",exit(EXIT_FAILURE));
            //aggiungiamo alla nostra maschera il descrittore del client
            //ed eventualmente aumentiamo il numero massimo di descrittori
            int* temp = malloc(sizeof(int));
            *temp= fd_c;
            push(s_queue,temp); //metto nella coda che esce dai workers

            FD_SET(fd,&set);
            if(fd_c > fd_num) fd_num = fd_c;
          }
          else if(fd == fd_select[0]) //come sopra, sto leggendo dal pipe di aggiornamento della select
          {
            IFERROR(read(fd_select[0],buf,1),"read fd_select");
            if(buf[0]=='0') continue;
            else if(buf[0]=='1')   //devo terminare i threads
            {
              for(int i=0;i<conf->ThreadsInPool;i++)
              {
                 int* t = malloc(sizeof(int));
                 *t= -1;
                 IFERROR(push(queue,t),"push() failed\n");

              }
              close(fd_sk);
              pthread_exit(NULL);
            }
          }
          else
          {
            //sock I/O pronto
            int* temp = malloc(sizeof(int));
            *temp= fd;
            FD_CLR(fd,&set);
            FD_CLR(fd,&rdset);
            descs[fd]=0;
            //metto in coda il descrittore sul quale devo operare
            IFERROR2(push(queue,temp),"push() failed\n",exit(EXIT_FAILURE));

          }
       }
       else
       {
         //lo rimetto nella maschera perchè deve ancora scrivere
         if(descs[fd]==1)
            FD_SET(fd,&set);
       }
    }
  }
  return NULL;
}


void* thread_worker(void* arg)
{
   int mythid = ((threadArgs_t*)arg)->thid; //thread id
   int* fd_l; //descrittore client
   //flag per le statistiche
   int flag=0;

   while(1)
   {
     flag=0; //resetto il flag
     message_t msg;  //messaggio da leggere
     //estraggo il descrittore
     fd_l =(int*)pop(queue);

     if(*fd_l==-1)   //se leggo l'intero -1 devo terminare il thread
     {
        fprintf(stdout,"Thread terminato #%d\n",mythid);
        free(fd_l);
        pthread_exit(NULL);
     }
     //leggo dal descrittore, se fallisco vuol dire che è offline
     if(readMsg(*fd_l,&(msg))==-1)
     {
       fprintf(stdout,"disconnesso descrittore #%d\n",*fd_l);
       if(disc_op(*fd_l,NULL))  nonline(-1); //lo setto offline
       close(*fd_l);
       free(fd_l);
       continue;
     }

     switch((op_t)(msg.hdr.op))  //in base alla op uso le funzioni di lib_utils
     {
        case REGISTER_OP:
              if(reg_op(*fd_l,&msg)==-1)
                  nerrors(1);
              else
              {
                nusers(1);
              }
              break;
        case CONNECT_OP:
             if(conn_op(*fd_l,&msg)==-1)
                 nerrors(1);
             else
             {
               nonline(1);
             }
             break;
        case DISCONNECT_OP:
             if(disc_op(*fd_l,&msg)==-1)
                 nerrors(1);
             break;
        case UNREGISTER_OP:
             if(unreg_op(*fd_l,&msg)==-1)
               nerrors(1);
             else
             {
               nusers(-1);
             }
             break;
        case USRLIST_OP:
             if(list_op(*fd_l,&msg)==-1)
                 nerrors(1);
             break;
        case POSTTXT_OP:
             if((flag=txt_op(*fd_l,&msg))==-1)
                 nerrors(1);
             break;
        case GETPREVMSGS_OP:
             if((flag=prvs_op(*fd_l,&msg))==-1)
                 nerrors(1);
             else
             {
               ndelivered(flag);
               nnotdelivered(- flag);
             }
             break;
        case POSTTXTALL_OP:
             if(txtall_op(*fd_l,&msg)==-1)
               nerrors(1);
             break;
        case POSTFILE_OP:
             if(file_op(*fd_l,&msg)==-1)
                 nerrors(1);
             else{

             }
             break;
        case GETFILE_OP:
             if(get_op(*fd_l,&msg)==-1)
                 nerrors(1);
             else{
               nfilenotdelivered(-1);
               nfiledelivered(1);
             }
               break;
        case CREATEGROUP_OP:
              if(create(&msg,*fd_l)==-1)
                  nerrors(1);
              else{

              }
              break;
        case ADDGROUP_OP:
              if(add(&msg,*fd_l)==-1)
                  nerrors(1);
              else{

              }
              break;
        case DELGROUP_OP:
              if(delete(&msg,*fd_l)==-1)
                  nerrors(1);
              else{

              }
              break;

        default:
             WRITE("op sconosciuta!");
             break;
     }
     push(s_queue,fd_l);  //rimetto il descrittore in coda
     IFERROR2(write(fd_select[1],"0",1),"write pipe failed\n",exit(EXIT_FAILURE));  //avverto la select che deve sbloccarsi
   }
   return NULL;
}

void server_start()
{

  init_utenti(); //inizializzo utenti e gruppi

  sockname=conf->UnixPath;

  //inizializzo code del thread listener
  queue=initQueue();
  s_queue=initQueue();

  //array di thread id
  pthread_t *th;
  MALLOC(th,((conf->ThreadsInPool)+1)*sizeof(pthread_t));

  //array parametri threads
  threadArgs_t* thARGS;
  MALLOC(thARGS,((conf->ThreadsInPool)+1)*sizeof(threadArgs_t));

  //inizializzo il thread listener
  thARGS[0].thid=0;
  pthread_create(&th[0],NULL,thread_listener,&thARGS[0]);

  //inizializzo i thread_worker
  for(int i=1;i<=(conf->ThreadsInPool);i++)
  {
     thARGS[i].thid=i;
     pthread_create(&th[i],NULL,thread_worker,&thARGS[i]);
  }

  //aspetto che finiscano
  for(int i=0;i<=(conf->ThreadsInPool);i++)
  {
     pthread_join(th[i],NULL);

  }
  //pulisco tutto
  delete_utenti();
  close(fd_select[0]);
  close(fd_select[1]);
  deleteQueue(queue);
  deleteQueue(s_queue);
  unlink(sockname);
  stat_del();
  free(th);
  free(thARGS);

}


void* signal_thread(void* arg)
{
    FILE *file;  //file per statistiche
    extern sigset_t signal_mask;
    int       sig_caught;  //segnale catturato
    while(1)
    {
        IFERROR2(sigwait( &signal_mask, &sig_caught),"sigwait error",exit(EXIT_FAILURE));
        switch (sig_caught)
        {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            IFERROR(write(fd_select[1],"1",1),"write pipe failed\n");  //scrivo al listener che deve terminare
            printf("devo terminare!\n");
            pthread_exit(NULL);
            break;
        case SIGUSR1:  //stampo statistiche
            file=fopen(conf->StatFileName,"a");
            printStats(file);
            fclose(file);
            break;
        case SIGSEGV:
            printf("ricevuto sigfault\n");
            break;
        default:
            fprintf (stderr, "\nSegnale sconosciuto %d\n", sig_caught);
            break;
        }
    }
    return NULL;
}

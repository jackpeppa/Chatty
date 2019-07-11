/**
 * @file lib_utils.c
 * @brief implementazione lib_utils.h
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#include "lib_utils.h"
//struttura di configurazione
extern configurazione* conf;
//array di mutex per sincronizzare la scrittura dei messaggi, ogni indice corrisponde ad un descrittore
static pthread_mutex_t* fds=NULL;
//lunghezza dell'array sopracitato
static int fds_len;
//mutex per l'array utenti
static pthread_mutex_t utenti_mtx;
//array di utenti(struttura dati principale per la gestione degli utenti)
static utente* utenti=NULL;
//lunghezza dell'array sopracitato
static int utenti_len=0;

//hashtable dei gruppi
icl_hash_t *hash;
//mutex dell' hashtable
static pthread_mutex_t groups_mtx;



void parse(char* fname, void* conf)
{
  int i=0;
  char*buf=malloc(LN*(sizeof(char)));
  CHECKNULL(buf,"malloc() error",exit(EXIT_FAILURE));

  FILE *file = fopen( fname, "r" );
  CHECKNULL(file,"impossibile aprire file config",exit(EXIT_FAILURE));

  while(i<8)
  {
    memset(buf,0,LN);
    fgets(buf,LN,file);
    if(buf[0]!='#' && buf[0]!= '\n')
    {
      char *token =strtok(buf,"=");
      char temp= token[3];

      token=strtok(NULL,"\n");
      char *token1=strtok(token," ");
      switch(temp)
      {
        case 'x':
            CHECKNULL(memcpy((((configurazione*)conf)->UnixPath),token1,strlen(token1)),"strcpy() error\n",exit(EXIT_FAILURE));
            break;
        case 'C':
            ((configurazione*)conf)->MaxConnections=atoi(token1);
            break;
        case 'e':
            ((configurazione*)conf)->ThreadsInPool=atoi(token1);
            break;
        case 'M':
            ((configurazione*)conf)->MaxMsgSize=atoi(token1);
            break;
        case 'F':
            ((configurazione*)conf)->MaxFileSize=atoi(token1)*1000;
            break;
        case 'H':
            ((configurazione*)conf)->MaxHistMsgs=atoi(token1);
            break;
        case 'N':
            CHECKNULL(memcpy((((configurazione*)conf)->DirName),token1,strlen(token1)),"strcpy() error\n",exit(EXIT_FAILURE));
            break;
        case 't':
            CHECKNULL(memcpy((((configurazione*)conf)->StatFileName),token1,strlen(token1)),"strcpy() error\n",exit(EXIT_FAILURE));
            break;
      }
      i++;
    }
  }
  free(buf);
  return;
}
//_________________________FUNZIONI PER GLI UTENTI_________________________________

/**
* @function cerca_nome
* @brief cerca l'utente che ha il campo nome==nome, se non c'è restituisce NULL
*/
static utente* cerca_nome(char* nome)
{
  for(int i=0;i<utenti_len;i++)
  {
    if((strcmp(utenti[i].nome,nome)==0))
    {
      return &utenti[i];
    }
  }
  return NULL;
}
/**
* @function cerca_descrittore
* @brief cerca se c'è un utente con il descrittore=fd e lo restituisce, se non c'è NULL
*/
static utente* cerca_descrittore(int fd)
{
  for(int i=0;i<utenti_len;i++)
  {
    if(utenti[i].fd==fd)
    {
      return &utenti[i];
    }
  }
  return NULL;
}
/**
* @function usrlist
* @brief crea stringa con lista degli utenti online, e ne mette il numero in sz
*/
static char* usrlist(int*sz)
{
  char* list=NULL;
  int dim=0;
  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock usrlist");
  for(int i=0;i<utenti_len;i++)
  {
    if(((utenti[i].used))==1 &&((utenti[i].online)==1))
    {
      dim++;
      REALLOC(list,dim*(MAX_NAME_LENGTH+1));
      memset(&list[(dim-1)*(MAX_NAME_LENGTH+1)],' ',MAX_NAME_LENGTH+1);
      strcpy(&list[(dim-1)*(MAX_NAME_LENGTH+1)],utenti[i].nome);
      memset(&list[(dim-1)*(MAX_NAME_LENGTH+1)+strlen(utenti[i].nome)],' ',1);
      list[(dim-1)*(MAX_NAME_LENGTH+1)+(MAX_NAME_LENGTH)]='\0';
    }
  }
  IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock usrlist");
  *sz=dim;
  return list;
}
/**
* @function resize_hist
* @brief toglie il messaggio piu vecchio dalla history e ci mette quello nuovo
* @param dest utente target
* @param msg_target messaggio da inserire nella history
*/
static void resize_hist(utente* dest,message_t* msg_target)
{
  message_t* old=&(dest->prvs[dest->oldest]);
  if(old)
     if(old->data.buf)
        free(old->data.buf);
  dest->prvs[dest->oldest]=*msg_target;
  dest->oldest=(dest->oldest+1)%(conf->MaxHistMsgs);
}
/**
* @function copy_message
* @brief crea una copia di src in dest
*/
static void copy_message(message_t* dest,message_t* src)
{
  dest->hdr.op=src->hdr.op;

  if(src->hdr.sender)
     strcpy(dest->hdr.sender,src->hdr.sender);

  if(src->data.buf)
     dest->data.buf=strdup(src->data.buf);

  if(src->data.hdr.receiver)
    strcpy(dest->data.hdr.receiver,src->data.hdr.receiver);

  dest->data.hdr.len=src->data.hdr.len;

}


// vede se due utenti hanno lo stesso nickname
int compara_utenti(void* u1,void* u2)
{
  if(strcmp(((utente*)u1)->nome,((utente*)u2)->nome)) return 1;
  return 0;
}

int reg_op(int fd,message_t* msg)
{
  char* buf=NULL;
  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock");
  if(cerca_nome(msg->hdr.sender))  //se c'è già mando errore
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock");
    setHeader(&(msg->hdr),OP_NICK_ALREADY,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
    IFERROR(sendAck(fd,msg),"sendack reg");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
    return -1;
  }
  else
  {
    int trovato=0;
    for(int i=0;i<utenti_len;i++)
    {
      if((utenti[i].used)==0) //se ho una cella libera nell'array utenti lo inserisco
      {
        memset((char*)(utenti[i].nome),0,MAX_NAME_LENGTH+1);
        CHECKNULL(strcpy(utenti[i].nome,msg->hdr.sender),"strcpy",return -1);
        utenti[i].used=1;
        if(utenti[i].prvs){
        for(int k=0;k<utenti[i].nprvs;k++)
        {
          if(&(utenti[i].prvs[k])){
          if(utenti[i].prvs[k].data.buf)
              free(utenti[i].prvs[k].data.buf);
            }
        }
            free(utenti[i].prvs);
        }
        utenti[i].prvs=NULL;
        utenti[i].nprvs=0;
        utenti[i].oldest=0;
        utenti[i].online=0;
        utenti[i].fd=fd;
        trovato=1;
        break;
      }
    }
    if(!trovato)     //se non c'è rialloco
    {
      REALLOC(utenti,(utenti_len+1)*sizeof(utente));
      memset(&(utenti[utenti_len]),0,sizeof(utente));
      utenti_len++;
      CHECKNULL(strcpy(utenti[utenti_len-1].nome,msg->hdr.sender),"strcpy",return -1);
      utenti[utenti_len-1].used=1;
      utenti[utenti_len-1].online=0;
      utenti[utenti_len-1].prvs=NULL;
      utenti[utenti_len-1].nprvs=0;
      utenti[utenti_len-1].oldest=0;
      utenti[utenti_len-1].fd=fd;

    }
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock");
    //mando esito e lista utenti
    setHeader(&(msg->hdr),OP_OK,"");
    int sz;
    buf=usrlist(&sz);
    if(buf)
      setData(&(msg->data),"", buf, sz*(MAX_NAME_LENGTH+1));
    else
      setData(&(msg->data),"", "", MAX_NAME_LENGTH+1);
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
    IFERROR(sendAck(fd,msg),"sendack reg");
    IFERROR(sendData(fd,&(msg->data)),"senddata reg op");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");

    free(buf);
  }
  return 1;
}


int conn_op(int fd,message_t* msg)
{
  utente* usr=NULL;  //utente da cercare
  char* buf=NULL;  //buffer per la lista utenti

  if(get_nonline()==conf->MaxConnections)  //se ho troppe connessioni rifiuto la richiesta
  {
    setHeader(&(msg->hdr),OP_FAIL,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock conn");
    IFERROR(sendAck(fd,msg),"sendack conn");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock conn");
    return -1;
  }

  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock conn_op");
  usr=cerca_nome(msg->hdr.sender);
  if(usr)  //se c'è lo setto online
  {
    usr->online=1;
    usr->fd=fd;
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock conn_op");
    setHeader(&(msg->hdr),OP_OK,"");
    int sz;
    buf=usrlist(&sz);
    setData(&(msg->data),"", buf, sz*(MAX_NAME_LENGTH+1));
  }
  else  //altrimenti mando OP_NICK_UNKNOWN
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock conn_op");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock conn");
    IFERROR(sendAck(fd,msg),"sendack conn");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock conn");
    return -1;
  }
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock conn");
  IFERROR(sendAck(fd,msg),"sendack conn");
  IFERROR(sendData(fd,&(msg->data)),"senddata conn");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock conn");
  free(buf);
  return 0;

}

int disc_op(int fd,message_t* msg)
{

  utente* usr=NULL;
  if(!msg)   //funzione utilizzata anche nel thread worker (con msg=NULL) per disconnettere un utente quando si legge 0 dal suo descrittore
  {
    int tmp=0;
    IFERROR(pthread_mutex_lock(&utenti_mtx),"lock disc_op");
    usr=cerca_descrittore(fd);
    if(usr){
      tmp=usr->online;
      usr->online=0;
      usr->fd=-1;
    }
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock disc_op");
    return tmp;
  }

  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock disc_op");
  usr=cerca_nome(msg->hdr.sender);
  if(usr)  //se c'è lo setto offline
  {
    usr->online=0;
    usr->fd=-1;
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock disc_op");
    setHeader(&(msg->hdr),OP_OK,"");
  }
  else  //mando OP_NICK_UNKNOWN
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock disc_op");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock disc");
    IFERROR(sendAck(fd,msg),"sendack disc");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock disc");
    return -1;
  }
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock disc");
  IFERROR(sendAck(fd,msg),"sendack disc");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock disc");
  return 0;
}

int unreg_op(int fd,message_t* msg)
{
  utente* usr=NULL; //utente da cercare
  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock unreg_op");
  usr=cerca_nome(msg->hdr.sender);
  if(usr)  //se c'è setto la cella come inutilizzata
  {
    memset(usr->nome,0,MAX_NAME_LENGTH+1);
    usr->used=0;
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock unreg_op");
    setHeader(&(msg->hdr),OP_OK,"");
  }
  else
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock unreg_op");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock unreg");
    IFERROR(sendAck(fd,msg),"sendack unreg");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock unreg");
    return -1;
  }
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock unreg");
  IFERROR(sendAck(fd,msg),"sendack unreg");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock unreg");
  return 0;
}

int list_op(int fd,message_t* msg)
{
  utente* usr=NULL;
  char* buf=NULL;
  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock list_op");
  usr=cerca_nome(msg->hdr.sender);
  IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock list_op");
  if(usr)  // se c'è genero la lista di utenti online
  {
    setHeader(&(msg->hdr),OP_OK,"");
    int sz;
    buf=usrlist(&sz);
    setData(&(msg->data),"", buf, sz*(MAX_NAME_LENGTH+1));
  }
  else
  {
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock list");
    IFERROR(sendAck(fd,msg),"sendack list");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock list");
    return -1;
  }
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock list");
  IFERROR(sendAck(fd,msg),"sendack list");
  IFERROR(sendData(fd,&(msg->data)),"senddata list");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock list");
  free(buf);
  return 0;
}

int txt_op(int fd,message_t* msg)
{
  if(msg->hdr.op!=FILE_MESSAGE)     //riutilizzo la funzione per i files, quindi non devrei fare questo controllo in quel caso
      if(msg->data.hdr.len>conf->MaxMsgSize)
      {
        setHeader(&(msg->hdr),OP_MSG_TOOLONG,"");
        IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
        IFERROR(sendAck(fd,msg),"sendack txt");
        IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");
        free(msg->data.buf);
        return -1;
      }

  int res=group_txt_op(msg,fd);  //gestisco la richiesta se è un groupname, altrimenti vado avanti
  if(res==0) return 0;
  else if(res==-1) return -1;
  //se non è groupname gestisco il messaggio come sempre

  utente* usr=NULL;   //sender
  utente* dest=NULL;  //receiver
  int desc;     //descrittore destinatario
  message_t msg_target;   //messaggio da inviare
  int temp=0;      //flag per controllare esito invio messaggio

  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock txt_op");
  usr=cerca_nome(msg->hdr.sender);
  dest=cerca_nome(msg->data.hdr.receiver);
  IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock txt_op");

  if(usr && dest)
  {
    //setto il messaggio da inviare
    if(msg->hdr.op!=FILE_MESSAGE){    //questo perché riutilizzo la funzione per i files
         setHeader(&(msg_target.hdr),TXT_MESSAGE,msg->hdr.sender);}
    else{
         setHeader(&(msg_target.hdr),FILE_MESSAGE,msg->hdr.sender);}
    setData(&(msg_target.data),msg->data.hdr.receiver,msg->data.buf,msg->data.hdr.len);

    if(dest->online==1) //se è online provo ad inviare
    {
      desc=dest->fd;
      IFERROR(pthread_mutex_lock(&fds[desc]),"lock txt");
      temp=sendAck(desc,&msg_target);
      sendData(desc,&(msg_target.data));
      IFERROR(pthread_mutex_unlock(&fds[desc]),"unlock txt");
      if(temp==-1)   //puo darsi che si sia disconnesso nel frattempo, in questo caso inserisco nella history
      {
        IFERROR(pthread_mutex_lock(&utenti_mtx),"lock txt_op");
        if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msg_target);  //se la history è piena faccio resize
        else
        {
            REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));

            (dest->prvs)[dest->nprvs]=msg_target;
            dest->nprvs++;
        }
        IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock txt_op");
        //statistiche
        if(msg->hdr.op!=FILE_MESSAGE) nnotdelivered(1);
        else nfilenotdelivered(1);
      }
      else{
        free(msg_target.data.buf);
        if(msg->hdr.op!=FILE_MESSAGE) ndelivered(1);
        else nfilenotdelivered(1);
        temp=0;
      }
    }
    else //se è offline metto nella history
    {
      IFERROR(pthread_mutex_lock(&utenti_mtx),"lock txt_op");
      if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msg_target);
      else
      {
          REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));
          (dest->prvs)[dest->nprvs]=msg_target;
          dest->nprvs++;
      }
      IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock txt_op");
      temp=1;
      if(msg->hdr.op!=FILE_MESSAGE) nnotdelivered(1);
      else nfilenotdelivered(1);
    }
    setHeader(&(msg->hdr),OP_OK,"");
  }
  else //se non trovo uno dei due nickname mando OP_NICK_UNKNOWN
  {
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
    IFERROR(sendAck(fd,msg),"sendack txt");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");
    free(msg->data.buf);
    return -1;
  }
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
  IFERROR(sendAck(fd,msg),"sendack txt");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");

  return 0;
}

int prvs_op(int fd,message_t* msg)
{
  utente* usr=NULL; //utente che deve ricevere la history
  message_t* prvs; //array messaggi history
  size_t nprvs; //lunghezza history

  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock prvs");
  usr=cerca_nome(msg->hdr.sender);
  if(usr)
  {
    prvs=usr->prvs;
    usr->prvs=NULL;
    nprvs=(size_t)usr->nprvs;
    usr->nprvs=0;
    usr->oldest=0;
    setHeader(&(msg->hdr),OP_OK,"");
    setData(&(msg->data),"",(const char*)&nprvs,sizeof(size_t));
    //invio OP_OK
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock prvs");
    IFERROR(sendAck(fd,msg),"sendack prvs");
    IFERROR(sendData(fd,&(msg->data)),"senddata prvs");
    //invio messaggi
    for(int i=0;i<nprvs;i++){
      IFERROR(sendAck(fd,&(prvs[i])),"sendack prvs");
      IFERROR(sendData(fd,&(prvs[i].data)),"senddata prvs");
      free(prvs[i].data.buf);
    }
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock prvs");
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock prvs");
    free(prvs);
  }
  else
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock prvs");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock prvs");
    IFERROR(sendAck(fd,msg),"sendack prvs");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock prvs");
    return -1;
  }
  return nprvs; //return numero messaggi inviati per le statistiche
}

int txtall_op(int fd,message_t* msg)
{
   int temp=0;    //var per controllare l'invio del messaggio
   message_t msg_target;  //messaggio da inviare
   int desc;    //descrittore temporaneo
   utente* dest=NULL;   //utente destinatario temporaneo
   int inviati=0;   //numero msg inviati (statistiche)
   int noninviati=0; //numero messaggi non inviati(statistiche)

   if(msg->data.hdr.len>conf->MaxMsgSize)
   {
     setHeader(&(msg->hdr),OP_MSG_TOOLONG,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
     IFERROR(sendAck(fd,msg),"sendack txt");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");
     free(msg->data.buf);
     return -1;
   }

   setHeader(&(msg_target.hdr),TXT_MESSAGE,msg->hdr.sender);
   setData(&(msg_target.data),msg->data.hdr.receiver,msg->data.buf,msg->data.hdr.len);
   IFERROR(pthread_mutex_lock(&utenti_mtx),"lock txtall");
   for(int i=0;i<utenti_len;i++)                                      //devo copiare tutti i messaggi altrimenti rischiano di essere cancellati!!!!
   {
         dest=&utenti[i];
         if(dest->used==0) continue;   //se è una posizione non registrata
         if(dest->fd==fd) continue;
         desc=utenti[i].fd;
         if(utenti[i].online==1)
         {
             IFERROR(pthread_mutex_lock(&fds[desc]),"lock txtall");
             sendAck(desc,&msg_target);
             temp=sendData(desc,&(msg_target.data));
             IFERROR(pthread_mutex_unlock(&fds[desc]),"unlock txtall");
             if(temp==-1)   //puo darsi che si sia disconnesso nel frattempo
             {
               message_t msgnew;
               memset(msgnew.hdr.sender,0,MAX_NAME_LENGTH+1);
               memset(msgnew.data.hdr.receiver,0,MAX_NAME_LENGTH+1);
               msgnew.data.buf=NULL;
               copy_message(&msgnew,&msg_target);
               if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msgnew);
               else
               {
                   REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));
                   (dest->prvs)[dest->nprvs]=msgnew;
                   dest->nprvs++;
               }
               temp=0;
               noninviati++;
             }
             else{
               inviati++;
             }
         }
         else  //se non è online
         {
             message_t msgnew;
             memset(msgnew.hdr.sender,0,MAX_NAME_LENGTH+1);
             memset(msgnew.data.hdr.receiver,0,MAX_NAME_LENGTH+1);
             msgnew.data.buf=NULL;
             copy_message(&msgnew,&msg_target);
             if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msgnew);
             else
             {
                 REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));
                 (dest->prvs)[dest->nprvs]=msgnew;
                 dest->nprvs++;
             }
             noninviati++;
         }
     }
   IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock txtall");
   setHeader(&(msg->hdr),OP_OK,"");
   free(msg_target.data.buf);
   IFERROR(pthread_mutex_lock(&fds[fd]),"lock txtall");
   IFERROR(sendAck(fd,msg),"sendack txtall");
   IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txtall");
   //statistiche
   ndelivered(inviati);
   nnotdelivered(noninviati);
   return 0;
}

int file_op(int fd,message_t* msg)
{
  //estraggo i nomi che mi servono (nome del file)
  char *token1=msg->data.buf;
  char* token2=NULL;
  char s[2]="/";
  token1 = strtok(msg->data.buf,s);
  while( token1 != NULL )
     {
        token2=token1;
        token1 = strtok(NULL,s);
     }
     size_t len= strlen(conf->UnixPath)+strlen(token2)+1;
     char* pathname=NULL; //pathname
     char* fname=NULL;   //filename
     MALLOC(pathname,len);
     memset(pathname, 0, len);
     MALLOC(fname,strlen(token2)+1);
     memset(fname,0,strlen(token2)+1);
     strcpy(fname,token2);
  sprintf(pathname,"%s/%s",conf->DirName,token2);

  message_t msg_target;  //messaggio da inviare al client che riceve il file
  setData(&(msg_target.data),msg->data.hdr.receiver,fname,strlen(fname)+1);
  FILE* file = fopen(pathname,"w");
  CHECKNULL(file,"fopen file_op",exit(errno));
  free(msg->data.buf);
  //leggo il file
  IFERROR(readData(fd,&(msg->data)),"readdata file_op");

  if(msg->data.hdr.len > conf->MaxFileSize) //se è troppo grande mando OP_MSG_TOOLONG
  {
    setHeader(&(msg_target.hdr),OP_MSG_TOOLONG,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock fd_op");
    IFERROR(sendAck(fd,&(msg_target)),"sendack fd_op");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock fd_op");
    free(pathname);
    free(msg->data.buf);
    fclose(file);
    free(fname);
    return -1;
  }
  //scrivo il file
  IFERROR(fwrite(msg->data.buf,1,msg->data.hdr.len,file),"fprintf file_op");
  fclose(file);
  setHeader(&(msg_target.hdr),FILE_MESSAGE,msg->hdr.sender);

  if(txt_op(fd,&msg_target)==-1) //avverto il client che può scaricare il file
  {
     unlink(pathname);
     free(pathname);
     free(msg->data.buf);
     free(fname);
     return -1;
  }
  free(msg->data.buf);
  free(pathname);
  return 0;
}

int get_op(int fd,message_t* msg)
{
     char* pathname=NULL; //path file
     char* fname=msg->data.buf; //nome file
     size_t len= strlen(conf->UnixPath)+strlen(msg->data.buf)+1;
     MALLOC(pathname,len);
     memset(pathname, 0, len);
     sprintf(pathname,"%s/%s",conf->DirName,fname);
     size_t  sz;  //size del file
     int descr;  //descrittore che mi serve per la mmap
     char* mappedfile=NULL;
     FILE* file = fopen(pathname,"r");
     if(!file) //se il file non c'è invio OP_NO_SUCH_FILE
     {
       setHeader(&(msg->hdr),OP_NO_SUCH_FILE,"");
       IFERROR(pthread_mutex_lock(&fds[fd]),"lock fd_op");
       IFERROR(sendAck(fd,msg),"sendack fd_op");
       IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock fd_op");
       free(msg->data.buf);
       fclose(file);
       free(pathname);
       return -1;
     }
     else
     {
       fseek(file, 0L, SEEK_END);
       sz = ftell(file);   //prendo la dimensione
       rewind(file);  //torno all'inizio
       descr=fileno(file);
       mappedfile = mmap(NULL, (int)sz, PROT_READ,MAP_PRIVATE, descr, 0);
       if (mappedfile == MAP_FAILED)
       {
     		   perror("mmap");
           fclose(file);
           free(fname);
           free(pathname);
     		   return -1;
 	     }

       setHeader(&(msg->hdr),OP_OK,"");
       setData(&(msg->data), "", mappedfile,strlen(mappedfile)+1);
       //mando il file
       IFERROR(pthread_mutex_lock(&fds[fd]),"lock get");
       IFERROR(sendAck(fd,msg),"sendack get");
       IFERROR(sendData(fd,&(msg->data)),"senddata get");
       IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock get");
       munmap(mappedfile, sz);
       fclose(file);
       free(fname);
       free(pathname);
       return 0;

     }

}


int create(message_t* msg,int fd)
{
   //se un gruppo con lo stesso nome già esiste invio OP_NICK_ALREADY
   IFERROR(pthread_mutex_lock(&groups_mtx),"lock groups");
   IFERROR(pthread_mutex_lock(&utenti_mtx),"lock groups");
   if(icl_hash_find(hash,msg->data.hdr.receiver)||cerca_nome(msg->data.hdr.receiver))
   {
     IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock groups");
     IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
     setHeader(&(msg->hdr),OP_NICK_ALREADY,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
     IFERROR(sendAck(fd,msg),"sendack reg");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
     return -1;
   }
   IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock groups");
   group* gruppo;
   MALLOC(gruppo,sizeof(group));
   memset(gruppo,0,sizeof(group));
   //inserisco gruppo nella tabella hash
   icl_hash_insert(hash, (void*)strdup(msg->data.hdr.receiver), (void*)gruppo);
   //metto l'amministratore nel gruppo
   CHECKNULL(strcpy(gruppo->nome,msg->data.hdr.receiver),"strcpy group",return -1);
   CHECKNULL(strcpy(gruppo->creatore,msg->hdr.sender),"strcpy group",return -1);
   MALLOC(gruppo->persone[0],MAX_NAME_LENGTH+1);
   memset(gruppo->persone[0],0,MAX_NAME_LENGTH+1);
   CHECKNULL(strcpy(gruppo->persone[0],msg->hdr.sender),"strcpy group",return -1);
   IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
   //invio ack
   setHeader(&(msg->hdr),OP_OK,"");
   IFERROR(pthread_mutex_lock(&fds[fd]),"lock conn");
   IFERROR(sendAck(fd,msg),"sendack conn");
   IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock conn");

   return 0;

}



int delete(message_t* msg,int fd)
{
   group* gruppo; //gruppo da cercare nella hashtable
   IFERROR(pthread_mutex_lock(&groups_mtx),"lock groups");
   gruppo=icl_hash_find(hash,msg->data.hdr.receiver);
   if(!gruppo)
   {
     IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
     setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
     IFERROR(sendAck(fd,msg),"sendack reg");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
     return -1;
   }
   int i;
   for(i=0;i<MAX_GROUP_DIM;i++) //cerco l'utente da eliminare dal gruppo
   {
     if(gruppo->persone[i])
       if(strcmp(gruppo->persone[i],msg->hdr.sender)==0)
       {
         free(gruppo->persone[i]);
         gruppo->persone[i]=NULL;
         break;
       }
   }
   IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");

   if(i==MAX_GROUP_DIM) //se non c'è nel gruppo
   {
     setHeader(&(msg->hdr),OP_FAIL,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
     IFERROR(sendAck(fd,msg),"sendack reg");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
     return -1;
   }

   setHeader(&(msg->hdr),OP_OK,"");
   IFERROR(pthread_mutex_lock(&fds[fd]),"lock conn");
   IFERROR(sendAck(fd,msg),"sendack conn");
   IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock conn");

   return 0;
}



int add(message_t* msg,int fd)
{
   group* gruppo; //gruppo da cercare nella hashtable
   IFERROR(pthread_mutex_lock(&groups_mtx),"lock groups");
   gruppo=icl_hash_find(hash,msg->data.hdr.receiver);
   if(!gruppo)
   {
     IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
     setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
     IFERROR(sendAck(fd,msg),"sendack reg");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
     return -1;
   }

   int i;
   for(i=0;i<MAX_GROUP_DIM;i++) //cerco una posizione libera nell'array del gruppo
   {
     if(gruppo->persone[i]==NULL)
     {
       gruppo->persone[i]=strdup(msg->hdr.sender);
       break;
     }
   }
   IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
   if(i==MAX_GROUP_DIM) //se il gruppo è pieno
   {
     setHeader(&(msg->hdr),OP_FAIL,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
     IFERROR(sendAck(fd,msg),"sendack reg");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
     return -1;
   }
   else //ok
   {
     setHeader(&(msg->hdr),OP_OK,"");
     IFERROR(pthread_mutex_lock(&fds[fd]),"lock groups");
     IFERROR(sendAck(fd,msg),"sendack groups");
     IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock groups");
   }

   return 0;
}

int group_txt_op(message_t* msg,int fd)
{
  utente* usr=NULL;   //sender
  utente* dest=NULL;  //receiver
  int desc;     //descrittore destinatario
  message_t msg_target;   //messaggio da inviare
  int temp=0;      //flag per controllare esito invio messaggio

  group* gruppo;
  IFERROR(pthread_mutex_lock(&groups_mtx),"lock groups");
  gruppo=icl_hash_find(hash,msg->data.hdr.receiver);
  if(!gruppo)
  {
    IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
    return 1;  //potrebbe essere un nickname e non un groupname
  }

  int i;
  for(i=0;i<MAX_GROUP_DIM;i++)
  {
    if(gruppo->persone[i])
      if(strcmp(gruppo->persone[i],msg->hdr.sender)==0)
         break;
  }
  if(i==MAX_GROUP_DIM) //se non c'è nel gruppo non può inviare msg
  {
    IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock");
    IFERROR(sendAck(fd,msg),"sendack reg");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock");
    free(msg->data.buf);
    return -1;
  }
  IFERROR(pthread_mutex_lock(&utenti_mtx),"lock groups");
  usr=cerca_nome(msg->hdr.sender);
  if(!usr)
  {
    IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock groups");
    IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
    setHeader(&(msg->hdr),OP_NICK_UNKNOWN,"");
    IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
    IFERROR(sendAck(fd,msg),"sendack txt");
    IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");
    free(msg->data.buf);
    return -1;
  }

  //inizio l'invio dei messaggi
  for(int z=0;z<MAX_GROUP_DIM;z++)
  {
    if(gruppo->persone[z])
    {
      dest=cerca_nome(gruppo->persone[z]);
      if(!dest)
      {
        free(gruppo->persone[z]);
        gruppo->persone[z]=NULL;
        continue; //l'utente del gruppo non è piu iscritto alla chat
      }
      else
      {
        if(msg->hdr.op!=FILE_MESSAGE){
             setHeader(&(msg_target.hdr),TXT_MESSAGE,msg->hdr.sender);}
        else{
             setHeader(&(msg_target.hdr),FILE_MESSAGE,msg->hdr.sender);}
        setData(&(msg_target.data),msg->data.hdr.receiver,msg->data.buf,msg->data.hdr.len);

        if(dest->online==1)  //provo ad inviare
        {
          desc=dest->fd;
          IFERROR(pthread_mutex_lock(&fds[desc]),"lock txt");
          sendAck(desc,&msg_target);
          temp=sendData(desc,&(msg_target.data));
          IFERROR(pthread_mutex_unlock(&fds[desc]),"unlock txt");
          if(temp==-1)   //puo darsi che si sia disconnesso nel frattempo
          {
            message_t msgnew;
            memset(msgnew.hdr.sender,0,MAX_NAME_LENGTH+1);
            memset(msgnew.data.hdr.receiver,0,MAX_NAME_LENGTH+1);
            msgnew.data.buf=NULL;
            copy_message(&msgnew,&msg_target);
            if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msgnew);  //lavoro sulla history
            else
            {
                REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));

                (dest->prvs)[dest->nprvs]=msgnew;
                dest->nprvs++;
            }
            if(msg->hdr.op!=FILE_MESSAGE) nnotdelivered(1);
            else nfilenotdelivered(1);

          }
          else
          {
            if(msg->hdr.op!=FILE_MESSAGE) ndelivered(1);
            else nfilenotdelivered(1);
            continue;
          }
        }
        else //se non era online metto nella history
        {
          message_t msgnew;
          memset(msgnew.hdr.sender,0,MAX_NAME_LENGTH+1);
          memset(msgnew.data.hdr.receiver,0,MAX_NAME_LENGTH+1);
          msgnew.data.buf=NULL;
          copy_message(&msgnew,&msg_target);
          if(dest->nprvs==conf->MaxHistMsgs) resize_hist(dest,&msgnew);
          else
          {
              REALLOC(dest->prvs,(dest->nprvs+1)*(sizeof(message_t)));
              (dest->prvs)[dest->nprvs]=msgnew;
              dest->nprvs++;
          }
          if(msg->hdr.op!=FILE_MESSAGE) nnotdelivered(1);
          else nfilenotdelivered(1);
        }
      }
    }
  }
  setHeader(&(msg->hdr),OP_OK,"");
  IFERROR(pthread_mutex_lock(&fds[fd]),"lock txt");
  IFERROR(sendAck(fd,msg),"sendack txt");
  IFERROR(pthread_mutex_unlock(&fds[fd]),"unlock txt");
  free(msg->data.buf);
  IFERROR(pthread_mutex_unlock(&utenti_mtx),"unlock groups");
  IFERROR(pthread_mutex_unlock(&groups_mtx),"unlock groups");
  return 0;
}



void init_utenti(void)
{
  //range descrittori da 0 a maxconnections+ 20 (potrebbero anche esserci descrittori di file)
  MALLOC(fds,(conf->MaxConnections+20)*sizeof(pthread_mutex_t));
  for(int i=0;i<(conf->MaxConnections+20);i++)
     IFERROR(pthread_mutex_init(&fds[i],NULL),"mutex_init");
     fds_len= conf->MaxConnections+20;

  //inizializzo mutex per array utenti
  IFERROR(pthread_mutex_init(&utenti_mtx,NULL),"mutex_init");


  //inizializzazione strutture per la gestione gruppi
  hash = icl_hash_create(conf->MaxConnections*30, NULL, NULL);
  IFERROR(pthread_mutex_init(&groups_mtx,NULL),"mutex_init");

}
//freekey e freegroup mi servono per pulire la hashtable alla chiusura di chatty
void freekey(void* s)
{
  if(s)
   free(s);
}

void freegroup(void* gruppo)
{
  for(int i=0;i<MAX_GROUP_DIM;i++)
  {
    if((char*)(((group*)gruppo)->persone[i])!=NULL)
    {
      free((char*)((group*)gruppo)->persone[i]);
    }
  }
  if(gruppo)
  free(gruppo);
}

void delete_utenti(void)
{
  //elimino descrittori scritture
  for(int i=0;i<fds_len;i++)
     IFERROR(pthread_mutex_destroy(&fds[i]),"mutex_destroy");
  if(fds)
    free(fds);

  IFERROR(pthread_mutex_destroy(&utenti_mtx),"mutex_destroy");

  for(int i=0;i<utenti_len;i++) //elimino utenti
  {
      for(int k=0;k<utenti[i].nprvs;k++)
      {
        if(utenti[i].prvs[k].data.buf)
           free(utenti[i].prvs[k].data.buf);
      }
      if(utenti[i].prvs)
        free(utenti[i].prvs);
  }
  //elimino gruppi
  void (*fk)(void*)= freekey;
  void (*fc)(void*)= freegroup;
  icl_hash_destroy(hash,fk,fc);
  IFERROR(pthread_mutex_destroy(&groups_mtx),"mutex_destroy");
}

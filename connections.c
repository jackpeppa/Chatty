/**
 * @file connections.c
 * @brief implementazione di connections.h
 * @author Giacomo Peparaio 530822
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 originale dell'autore
 */
#include "connections.h"



int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
    int fd_skt;
    struct sockaddr_un sa;

    strncpy(sa.sun_path,path,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;
    fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
    IFERROR(fd_skt,"socket()");

    while (connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1)
    {
        if(ntimes==0) return -1;
        if(errno==ENOENT)  //non esiste,deve essere ancora creato
          sleep(secs);
        else return -1;
        ntimes--;
    }
    return fd_skt;
}


int readHeader(int connfd, message_hdr_t *hdr)
{
  op_t temp;
  char sender[MAX_NAME_LENGTH+1];
  if(read(connfd,&temp,sizeof(op_t))<=0) return -1;
  if(read(connfd,sender,MAX_NAME_LENGTH+1)<=0) return -1;
  setHeader(hdr,temp,sender);
  return 1;

}

int readData(long fd, message_data_t *data)
{
  char receiver[MAX_NAME_LENGTH+1];
  char* buf=NULL;
  size_t sz;
  if(read(fd,receiver,MAX_NAME_LENGTH+1)<0) return -1;
  if(read(fd,&sz,sizeof(size_t))<0) return -1;
  if(sz>0)
  {
    MALLOC(buf,(int)sz);
    memset(buf,0,(int)sz);
    if(read(fd,buf,(int)sz)<0) return -1;
  }
  setData(data,receiver,buf,sz);

  return 1;
}


int readMsg(long fd, message_t *msg)
{
  op_t temp;
  size_t sz;
  char sender[MAX_NAME_LENGTH+1];
  char receiver[MAX_NAME_LENGTH+1];
  memset(sender,0,MAX_NAME_LENGTH+1);
  memset(receiver,0,MAX_NAME_LENGTH+1);
  char* buf=NULL;

  if(read(fd,&temp,sizeof(op_t))<=0) return -1;
  if(read(fd,sender,MAX_NAME_LENGTH+1)<=0) return -1;
  setHeader(&(msg->hdr),temp,sender);

  if(read(fd,receiver,MAX_NAME_LENGTH+1)<0) return -1;
  if(read(fd,&sz,sizeof(size_t))<0) return -1;
  if(sz>0)
  {
    MALLOC(buf,(int)sz);
    memset(buf,0,(int)sz);
    if(read(fd,buf,(int)sz)<0) return -1;
  }
  setData(&(msg->data),receiver,buf,sz);
  return 1;
}

int sendAck(int fd, message_t *msg)
{
  if(write(fd,&(msg->hdr.op),sizeof(op_t))<=0) return -1;
  if(write(fd,(char*)(*msg).hdr.sender,MAX_NAME_LENGTH+1)<=0) return -1;
  return 1;
}


int sendRequest(long fd, message_t *msg)
{
  if(write(fd,&(msg->hdr.op),sizeof(op_t))<=0) return -1;
  if(write(fd,(char*)(*msg).hdr.sender,MAX_NAME_LENGTH+1)<=0) return -1;
  if(write(fd,(char*)(*msg).data.hdr.receiver,MAX_NAME_LENGTH+1)<0) return -1;

  if(write(fd,&((*msg).data.hdr.len),sizeof(size_t))<0) return -1;
  if((*msg).data.hdr.len>0)
  {
     if(write(fd,(char*)(*msg).data.buf,(*msg).data.hdr.len)<0) return -1;
  }
  return 1;

}


int sendData(long fd, message_data_t *msg)
{
  if(write(fd,(char*)(*msg).hdr.receiver,MAX_NAME_LENGTH+1)<=0) return -1;
  if(write(fd,&((*msg).hdr.len),sizeof(size_t))<=0) return -1;
  if((*msg).hdr.len>0)
  {
  if(write(fd,(*msg).buf,(*msg).hdr.len)<=0) return -1;
  }
  return 1;

}

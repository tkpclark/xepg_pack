#include "sk.h"

void fatal(char *text)
{
exit(1);
}
skt_s *sopen(void)
{
skt_s *sp;
if((sp=(skt_s *)malloc(sizeof(skt_s)))==0)
   return 0;
if((sp->sd=socket(AF_INET,SOCK_STREAM,0))==-1)
{
  free(sp);
  return 0;
}
sp->sinlen=sizeof(sp->sin);
sp->bindflag=0;
return sp;
}

sclose(skt_s *sp)
{
  int sd;
  sd=sp->sd;
  free(sp);   
  return close(sd);
}

sclient(skt_s *sp,char *name,int port)
{
  struct  hostent *he;
  strcpy(sp->ip,name);
  sp->port=port;
  if((he=gethostbyname(name))==0)
    return -1;
  sp->sin.sin_family=AF_INET;
  sp->sin.sin_port=htons((unsigned short)port);
  sp->sin.sin_addr.s_addr=*(unsigned long *)he->h_addr;  
  bzero(&(sp->sin.sin_zero),8);
  if(connect(sp->sd,(struct sockaddr *)&sp->sin,sp->sinlen)==-1)
    return -1;
  return sp->sd;
}


sbind(skt_s *sp,int port,int sync)
{
int flags;
   sp->sin.sin_family=AF_INET;
   sp->sin.sin_port=htons((unsigned short)port);
   //sp->sin.sin_addr.s_addr=*(unsigned long *)he->h_addr;
   sp->sin.sin_addr.s_addr=htonl(INADDR_ANY);
   bzero(&(sp->sin.sin_zero),8);
   if(bind(sp->sd,(struct sockaddr *)&sp->sin,sp->sinlen)==-1
       ||  listen(sp->sd,1)==-1)
      return -1;
   sp->bindflag=1;
switch(sync)
{
  case 0:
     if((flags=fcntl(sp->sd,F_GETFL))==-1 || fcntl(sp->sd,F_SETFL,flags&~O_NDELAY)==-1)
     return -1;

     break;
  case 1:
     if((flags=fcntl(sp->sd,F_GETFL))==-1 || fcntl(sp->sd,F_SETFL,flags|O_NDELAY)==-1)
       return -1;
     break;
  default:
     return -1;
}
 return 0;
}


sserver(skt_s *sp,int port,int sync)
{
int flags;
//struct hostent *he;
//char localhost[64+1];
if(sp->bindflag==0)
{
  // if(gethostname(localhost,64)==-1 || (he=gethostbyname(localhost))==0)
    //    return -1;
   //printf("host:%s\n",localhost);     
   sp->sin.sin_family=AF_INET;
   sp->sin.sin_port=htons((unsigned short)port);
   //sp->sin.sin_addr.s_addr=*(unsigned long *)he->h_addr;  
   sp->sin.sin_addr.s_addr=htonl(INADDR_ANY);  
   bzero(&(sp->sin.sin_zero),8);       
   if(bind(sp->sd,(struct sockaddr *)&sp->sin,sp->sinlen)==-1
       ||  listen(sp->sd,5)==-1)
      return -1;
   sp->bindflag=1;
}
switch(sync)
{
  case 0:
     if((flags=fcntl(sp->sd,F_GETFL))==-1 || fcntl(sp->sd,F_SETFL,flags&~O_NDELAY)==-1)
     return -1;

     break;         
  case 1:
     if((flags=fcntl(sp->sd,F_GETFL))==-1 || fcntl(sp->sd,F_SETFL,flags|O_NDELAY)==-1)
       return -1;
     break;
  default:
     return -1;
}
//printf("waiting ... port:%d\n",port);
return accept(sp->sd,(struct sockaddr *)&sp->sin,&sp->sinlen);
}            
short writeall(int sd,char *buf,int num)
{
int j;
lp:
if((j=write(sd,buf,num))!=num)
   if(j==-1)
     if(errno==EWOULDBLOCK || errno==EAGAIN)
      goto lp;
     else
      return(-1);
   else
     { 
       num-=j;
       buf+=j;       
       goto lp;
     }
return 0;
}
skt_s* tconnect(char *dip,short dport,int maxredo)
{
	skt_s *sp;
	int redo=0;
	struct timeval wtime={3,0};
	if(!(sp=(skt_s*)sopen()))
	{
		return NULL;
	}
	setsockopt(sp->sd,SOL_SOCKET,SO_SNDTIMEO,&wtime,sizeof(wtime));
s:
	 if(sclient(sp,dip,dport)==-1) 
	{
		redo++;
		if(redo>=maxredo)
			return NULL;
		sleep(1);
		goto s;
	}
	return sp;
}
/*
int tsend(skt_s *sp,char *p_data,int len,int maxredo)
{
	int n;
	skt_s *mysp;
	if(!len)
	{
		return 0;
	}
w:
	if((n=send(sp->sd,p_data,len,0))!=len)
	{
		mysp=tconnect(sp->ip,sp->port,maxredo);
		if(!mysp)
			return -1;
		memcpy(sp,mysp,sizeof(skt_s));
		goto w;
	}
	return len;
}
*///wrong when you need to trans more than 2048 bytes


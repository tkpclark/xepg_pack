#include"DST_send.h"
#include"sk.h"
static char logbuf[2048];
extern int logfd;
extern char mdname[];



void DST_com_header(DST_HDR *p_dst_hdr,DST_STREAM *p_dst_stream)
{
	//cmd
	*(int*)(p_dst_stream->buffer+DST_CMD_OFFS)=htonl(p_dst_hdr->cmd);
	
	//client id
	*(int*)(p_dst_stream->buffer+DST_CLTID_OFFS)=htonl(p_dst_hdr->cltid);

	//key
	*(int*)(p_dst_stream->buffer+DST_CLTKEY_OFFS)=htonl(p_dst_hdr->cltkey);
}


void DST_parse_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream)
{
	p_dst_resp->code=ntohl(*(int*)(p_dst_stream->buffer+DST_CODE_OFFS));
	//free(p_dst_stream->buffer);
}

void DST_com_cmd1(DST_CMD1 *p_dst_cmd1,DST_STREAM *p_dst_stream)
{
	p_dst_stream->buffer=malloc(p_dst_cmd1->ts_len+DST_TS_BUF_OFFS);
	//DST_HDR
	DST_com_header(p_dst_cmd1->p_dst_hdr,  p_dst_stream);

	//DeviceID
	*(int*)(p_dst_stream->buffer+DST_DEVID_OFFS)=htonl(p_dst_cmd1->DeviceId);

	//ChannelID
	*(int*)(p_dst_stream->buffer+DST_CHNID_OFFS)=htonl(p_dst_cmd1->ChannelId);
	
	//interval
	*(int*)(p_dst_stream->buffer+DST_INTERVAL_OFFS)=htonl(p_dst_cmd1->interval);

	//stream
	*(unsigned short*)(p_dst_stream->buffer+DST_STREAM_OFFS)=htons(p_dst_cmd1->stream_id);
	
	//base
	*(int*)(p_dst_stream->buffer+DST_BASE_OFFS)=htonl(p_dst_cmd1->base_id);
	
	//length
	*(int*)(p_dst_stream->buffer+DST_TS_LEN_OFFS)=htonl(p_dst_cmd1->ts_len);

	//ts_buffer
	memcpy(p_dst_stream->buffer+DST_TS_BUF_OFFS,p_dst_cmd1->ts_buffer,p_dst_cmd1->ts_len);

	p_dst_stream->len=DST_TS_BUF_OFFS+p_dst_cmd1->ts_len;
}


void DST_com_cmd10(DST_CMD10 *p_dst_cmd10,DST_STREAM *p_dst_stream)
{
	//DST_HDR
	p_dst_stream->buffer=malloc(DST_HDR_LEN);
	DST_com_header(p_dst_cmd10->p_dst_hdr,  p_dst_stream);
	p_dst_stream->len=DST_HDR_LEN;
}

void DST_com_cmd2(DST_CMD2 *p_dst_cmd2,DST_STREAM *p_dst_stream)
{
	//DST_HDR
	p_dst_stream->buffer=malloc(DST_HDR_LEN);
	DST_com_header(p_dst_cmd2->p_dst_hdr,  p_dst_stream);
	p_dst_stream->len=DST_HDR_LEN;
}

void DST_com_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream)
{
	*(int*)(p_dst_stream->buffer+DST_CODE_OFFS)=htonl(p_dst_resp->code);
	p_dst_stream->len=4;
	
}


int DST_send_data(DST_STREAM *p_dst_stream,char *dip,short dport)
{
	skt_s *sp;
	int maxredo=2;
	int n=0;
	int send_bytes=0;

	int ret=0;


	DST_STREAM dst_stream_resp;
	DST_RESP dst_resp;
	dst_stream_resp.buffer=malloc(sizeof(DST_RESP));


	
	sp=tconnect(dip,dport,maxredo);
	if(!sp)
	{
		sprintf(logbuf,"连接DS服务器失败[%s|%d]!\n",dip,dport);
		proclog(logfd,mdname,logbuf);
		ret=-4;
		goto freeall;
	}
	//send data

	
	n=writeall(sp->sd, p_dst_stream->buffer, p_dst_stream->len);
	if(n<0)
	{
		sprintf(logbuf,"发送数据失败[%s|%d]!\n",dip,dport);
		proclog(logfd,mdname,logbuf);
		ret=-2;
		goto freeall;
	}
	//printf("send %d bytes to ds server!\n",n);



	/////////////read resp
	
	
	if((dst_stream_resp.len=read(sp->sd,dst_stream_resp.buffer,sizeof(DST_RESP)))!=sizeof(DST_RESP))
	{
		sprintf(logbuf,"读取DS应答失败[%s|%d]!\n",dip,dport);
		proclog(logfd,mdname,logbuf);
		ret=-3;
		goto freeall;
	}

	DST_parse_resp(&dst_resp, &dst_stream_resp);
	ret=dst_resp.code;
	
	if(dst_resp.code>=-1)
	{
		;//proclog(logfd,mdname,"命令执行成功!");
	}
	else
	{
		sprintf(logbuf,"任务解析失败!错误代码:%d\n",dst_resp.code);
		proclog(logfd,mdname,logbuf);
		ret=-4;
		goto freeall;
	}
	


freeall:
	free(p_dst_stream->buffer);
	free(dst_stream_resp.buffer);
	if(sp!=NULL)
		sclose(sp);

	return ret;
}
int DST_send_cmd1(int cltid,int interval,int extend_id,unsigned int baseid,char *ip,short port)
{
	int i;
	char temp[128];
	unsigned int max_ts_file=1024*30000;
	char *ts_buffer_temp=NULL;
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=1;
	
	DST_CMD1 dst_cmd1;
	dst_cmd1.ChannelId=0;
	dst_cmd1.DeviceId=0;
	dst_cmd1.interval=interval;
	dst_cmd1.stream_id=extend_id;
	dst_cmd1.base_id=baseid;
	dst_cmd1.p_dst_hdr=&dst_hdr;


	//get ts file info
	int fd;
	sprintf(temp,"../taskts/%d.ts",cltid);
	fd=open(temp,0);
	if(fd<0)
	{
		sprintf(logbuf,"打开ts文件%s失败! %s\n",temp,strerror(errno));
		proclog(logfd,mdname,logbuf);
		close(fd);

		if(errno==EFAULT)
			exit(0);
		return -1;
	}
	ts_buffer_temp=malloc(max_ts_file);
	dst_cmd1.ts_len=read(fd,ts_buffer_temp,max_ts_file);
	if(dst_cmd1.ts_len<=0)
	{
		
		sprintf(logbuf,"读取ts文件%s失败 ,return %d! %s\n",temp,dst_cmd1.ts_len,strerror(errno));
		proclog(logfd,mdname,logbuf);
		close(fd);
		free(ts_buffer_temp);
		return -2;
	}
	close(fd);
	dst_cmd1.ts_buffer=malloc(dst_cmd1.ts_len);
	memcpy(dst_cmd1.ts_buffer,ts_buffer_temp,dst_cmd1.ts_len);
	free(ts_buffer_temp);

	DST_STREAM dst_stream;
	DST_com_cmd1(&dst_cmd1, &dst_stream);

	
	free(dst_cmd1.ts_buffer);

	sprintf(logbuf,"发布任务:cltid[%d]ip[%s:%d]cmd[%d]baseid[%d]ts[%d]len[%d]interval[%d]",
		dst_cmd1.p_dst_hdr->cltid,
		ip,
		port,
		dst_cmd1.p_dst_hdr->cmd,
		dst_cmd1.base_id,
		dst_cmd1.stream_id,
		dst_cmd1.ts_len,
		dst_cmd1.interval
		);
	proclog(logfd,mdname,logbuf);


	

	return DST_send_data(&dst_stream,ip, port);
	
}

int DST_send_cmd10(int cltid,char *ip,short port)//stop a task
{
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=10;

	DST_CMD10 dst_cmd10;
	dst_cmd10.p_dst_hdr=&dst_hdr;

	DST_STREAM dst_stream;
	DST_com_cmd10(&dst_cmd10, &dst_stream);

	sprintf(logbuf,"停止任务:cltid[%d]ip[%s]port[%d]cmd[%d]key[%d]",
		dst_cmd10.p_dst_hdr->cltid,
		ip,
		port,
		dst_cmd10.p_dst_hdr->cmd,
		dst_cmd10.p_dst_hdr->cltkey);
	proclog(logfd,mdname,logbuf);
	
	return DST_send_data(&dst_stream,ip, port);

}

int DST_send_cmd2(int cltid,char *ip,short port)//querya task
{
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=2;
	int n;

	DST_CMD2 dst_cmd2;
	dst_cmd2.p_dst_hdr=&dst_hdr;

	DST_STREAM dst_stream;
	DST_com_cmd2(&dst_cmd2, &dst_stream);

	/*
	sprintf(logbuf,"查询任务状态:cltid[%d]ip[%s]port[%d]cmd[%d]key[%d]",
		dst_cmd2.p_dst_hdr->cltid,
		ip,
		port,
		dst_cmd2.p_dst_hdr->cmd,
		dst_cmd2.p_dst_hdr->cltkey);
	proclog(logfd,mdname,logbuf);
	*/
	n=DST_send_data(&dst_stream,ip, port);
	/*
	if(n<-1)
	{
		proclog(logfd,mdname,"查询出错:%d");
	}
	else if(n==-1)
	{
		proclog(logfd,mdname,"DS无此任务在运行!");
	}
	else if(n==1)
	{
		proclog(logfd,mdname,"DS有此任务在运行!");
	}
	*/
	
}



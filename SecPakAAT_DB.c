#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_AAT_SECTION_LEN 4096

static char logbuf[2048];
MYSQL mysql;
int logfd=0;
char mdname[]="AAT";
static char sql[512];
static unsigned int baseid=51;
static unsigned char table_id=0xAA;
static unsigned short network_id=0;
static int networkID;

unsigned char DTB_table_id=0;
char ds_server[32];
int ds_port=0;
char DTB_all_flag=0;
char DTB_stream_id[32];
unsigned int printlevel = 0;

static char CreateAAS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number)
{
	printf("+++++section_number:%d+++++\n",section_number);
	p->buffer=malloc(MAX_AAT_SECTION_LEN);
	memset(p->buffer,0,MAX_AAT_SECTION_LEN);
	
	int i,j;
	int num=0;
	unsigned int offs=0;
	unsigned int map_length_offs=0;

	//unsigned int transport_stream_loop_length_offs=0;
	//unsigned int transport_descriptors_length_offs=0;
	
	AAT_HEADER aat_header;
	
	memcpy(aat_header.table_id,"00000000",sizeof(aat_header.table_id));
	memcpy(aat_header.section_syntax_indicator,"1",sizeof(aat_header.section_syntax_indicator));
	memcpy(aat_header.reserved_future_use0,"1",sizeof(aat_header.reserved_future_use0));
	memcpy(aat_header.reserved0,"11",sizeof(aat_header.reserved0));
	memcpy(aat_header.section_length,"000000000000",sizeof(aat_header.section_length));
	memcpy(aat_header.reserved1,"1111111111111111",sizeof(aat_header.reserved1));
	memcpy(aat_header.reserved2,"11",sizeof(aat_header.reserved2));
	memcpy(aat_header.version_number,"00000",sizeof(aat_header.version_number));
	memcpy(aat_header.current_next_indicator,"1",sizeof(aat_header.current_next_indicator));
	memcpy(aat_header.section_number,"00000000",sizeof(aat_header.section_number));
	memcpy(aat_header.last_section_number,"00000000",sizeof(aat_header.last_section_number));
	compose_asst((char*)&aat_header,sizeof(aat_header),"AAT HEADER",p->buffer+offs);
	offs+=sizeof(aat_header)/8;
	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	
	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));
	
	
	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;

	/**********  BODY **********/
	//map_version
	*(unsigned short *)(p->buffer+offs)=htons(version_number+0x1f00);
	offs+=2;
	
	//map_length
	map_length_offs=offs;
	offs+=2;

	/*
	for(i=0;i<N;i++){
	 	 service_id
	  	author_count
	  	For(j=0;j<N;j++)
	  	{
	   		author_code
	 	}
	}
	*/
	char data_1[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	char data_service[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	int num_1=0;
	sprintf(sql,"select DISTINCT service_id from xepg_aat_extension");
	num=mysql_get_data(&mysql, sql,data_service);
	for(i=0;i<num;i++)
	{
		//serviceid
		sprintf(sql,"select service_id from xepg_sdt_extension where id=%d",atoi(data_service[i][0]));
		mysql_get_data(&mysql, sql,data);
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data[0][0]));
		offs+=2;

		sprintf(sql,"select author_code from xepg_aat_extension where service_id=%d",atoi(data_service[i][0]));
		num_1=mysql_get_data(&mysql, sql,data_1);
		//author_count
		*(unsigned char *)(p->buffer+offs)=(unsigned char)num_1;
		offs++;
		//author_code
		for(j=0;j<num_1;j++)
		{
			*(unsigned int *)(p->buffer+offs)=htonl(atoi(data_1[j][0]));
			offs+=4;
		}
	}
	*(unsigned short *)(p->buffer+map_length_offs)=htons(offs-map_length_offs-2);
	
	
	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("section_length:%d ",offs+4-3);
	print_HEX(p->buffer+1,2);

	offs+=4;//for CRC
	p->len=offs;
	return 0;
	
}
static write_section(SEC_BUFFER_INFO* p,int sec_num)
{
	int fd;
	fd=open("sec.bin",O_CREAT|O_TRUNC|O_WRONLY,0600);
	if(fd<0)
	{
		printf("open failed !\n");
		exit(0);
	}
	int i;
	for(i=0;i<sec_num;i++)
	{
		write(fd,p[i].buffer,p[i].len);
	}
	close(fd);
}
static int CreateOneTable_AAT(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id)
{
	int ret;
	int num=0;
	unsigned char section_number=0;

	//version_number
	unsigned char version_number=0;
	version_number=get_version_number(table_id, baseid, streamID);
	//printf("version number:%d\n",version_number);
	
	while(1)
	{
		
		ret=CreateAAS(table_id,section_number,&p_sbi[section_number],version_number);
		section_number++;
		printf("\n\n");
		
		if(!ret)
			break;
		
	}

	/* update last section number of every section */
	int i;
	for(i=0;i<section_number;i++)
	{
		*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}
	
	PrintToFile(logfd,mdname,"ver[%d]ts[%s]tb[0x%x]sec[%d]",version_number,stream_id,table_id,section_number);
	return section_number;
}

static DTBOneTable_AAT(char *streamID,char *stream_id)
{
	int num=0;
	printf("Distribute currently: table:%d(0x%x) in stream:%s\n",table_id,table_id,stream_id);
	/************Check IF DTB THIS TABLE**************/
	if( (!DTB_all_flag) && ( strcmp(stream_id,DTB_stream_id) || (table_id!=DTB_table_id) ) )
		return;
	

	unsigned short pid=0;
	int cltid=0;
	int interval=0;

	/********** Get Distribute conditions**************/
	if(get_dtb_info(&pid,&cltid,&interval,baseid,streamID,stream_id,table_id))
		return;

	/*****update packing flag*****/
	update_packing_flag(1,pid,table_id,baseid);
	
	/***********Create SECTIONS*****************/
	SEC_BUFFER_INFO sbi[256];
	memset(sbi,0,256*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	section_number=CreateOneTable_AAT(table_id,sbi,streamID,stream_id);
	
	/************PACK************/
	pack_and_dtb(section_number, sbi, baseid,table_id,ds_server,ds_port,pid,cltid,interval,stream_id);

	/*****update packing flag*****/
	update_packing_flag(0,pid,table_id,baseid);
}
static DTBAAT()
{
	int i;
	int stream_num=0;
	char data_stream[50][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	
	//get network_id
	get_network_id(&networkID,&network_id);
	
	//get all streams
	stream_num=get_all_streams(networkID,data_stream);
	
	for(i=0;i<stream_num;i++)
	{
		printf("########stream:%s#########\n",data_stream[i][0]);
		DTBOneTable_AAT(data_stream[i][0],data_stream[i][1]);
		
	}
}


main(int argc,char **argv)
{
	read_argv_1(argc, argv);
	pack_init();
	DTBAAT();
}

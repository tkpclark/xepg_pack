#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"

#define MAX_PMT_SECTION_LEN 1024
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="PMT";
static char sql[512] = {0};
static unsigned int baseid=8;


//static unsigned char DTB_table_id=0;
static unsigned char PMT_table_id = 0x02;

char ds_server[32];
int ds_port=0;
char DTB_all_flag=0;
char DTB_stream_id[32];
unsigned char DTB_table_id;

unsigned int printlevel = 0;


typedef union
{
	unsigned short	shortbyte;
	struct
	{
		unsigned char lowbyte;
		unsigned char highbyte;	
	}byte;
}SHORT2BYTE;

static char CreatePMS(unsigned char table_id,char *service_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number)
{
	printf("+++++section_number:%d+++++\n",section_number);
	p->buffer=malloc(MAX_PMT_SECTION_LEN);
	memset(p->buffer,0,MAX_PMT_SECTION_LEN);
	
	int i;
	unsigned int offs=0;
	int num=0,num1 = 0;

	/******HEADER*******/
	NIT_HEADER pmt_header;
	
	memcpy(pmt_header.table_id,"00000000",sizeof(pmt_header.table_id));
	memcpy(pmt_header.section_syntax_indicator,"1",sizeof(pmt_header.section_syntax_indicator));
	memcpy(pmt_header.reserved_future_use0,"0",sizeof(pmt_header.reserved_future_use0));
	memcpy(pmt_header.reserved0,"11",sizeof(pmt_header.reserved0));
	memcpy(pmt_header.section_length,"000000000000",sizeof(pmt_header.section_length));
	memcpy(pmt_header.network_id,"0000000000000000",sizeof(pmt_header.network_id));
	memcpy(pmt_header.reserved1,"11",sizeof(pmt_header.reserved1));
	memcpy(pmt_header.version_number,"00000",sizeof(pmt_header.version_number));
	memcpy(pmt_header.current_next_indicator,"1",sizeof(pmt_header.current_next_indicator));
	memcpy(pmt_header.section_number,"00000000",sizeof(pmt_header.section_number));
	memcpy(pmt_header.last_section_number,"00000000",sizeof(pmt_header.last_section_number));
	
	compose_asst((char*)&pmt_header,sizeof(pmt_header),"PMT HEADER",p->buffer+offs);
	offs+=sizeof(pmt_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//transport_stream_id
	
	*(unsigned short *)(p->buffer+3)=htons(atoi(service_id));  // transport_stream_id:0 ,before multiplexer,only one stream

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;

	//pcr pid
	sprintf(sql,"SELECT pcr_pid from xepg_sdt_extension where service_id=%s",service_id);
	char data_stream[1][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	num=mysql_get_data(&mysql, sql,data_stream);
	*(unsigned short*)(p->buffer+offs)=htons((unsigned short)atoi(data_stream[0][0]));
	offs+=2;

	//program info length
	*(unsigned short*)(p->buffer+offs)=0;
	offs+=2;
	/******BODY*******/
	
	/* loop of program component */
	//component number
	sprintf(sql,"SELECT elementary_pid,stream_type from xepg_pmt_extension \
			where service_id=(select id from xepg_sdt_extension where service_id \
			=%s)",service_id);
	char data_stream1[10][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	num1=mysql_get_data(&mysql, sql,data_stream1);

	for(i=0;i<num1;i++)
	{
		//stream_type
		*(unsigned char *)(p->buffer+offs) = atoi(data_stream1[i][1]);
		offs++;

		//elementary pid
		*(unsigned short *)(p->buffer+offs) = htons((unsigned short)atoi(data_stream1[i][0]));
		offs+=2;

		//es info length
		*(unsigned short *)(p->buffer+offs) = 0;
		offs+=2;
	}

	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}



static int CreateOneTable_PMT(SEC_BUFFER_INFO *p_sbi,char *service_id)
{
	int ret;
	int num=0;
	unsigned char section_number=0;

	//version_number
	unsigned char version_number=0;
	version_number=0;//get_version_number(table_id, baseid, 0);
	
	while(1)
	{
		
		ret=CreatePMS(PMT_table_id,service_id,section_number,&p_sbi[section_number],version_number);
		section_number++;
		printf("\n\n");
		
		if(!ret)
			break;
		
	}

	/* update last section number of every section */
	int i;
	for(i=0;i<section_number;i++)
	{
		//*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}
	/*
	sprintf(logbuf,"ver[%d]ts[0]tb[0x%x]sec[1]",version_number,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	PrintToFile(logfd,mdname, "ver[%d]ts[0]tb[0x%x]sec[1]",version_number,PMT_table_id);
	return section_number;
}
static DTBOneTable_PMT(char *serviceID,char *service_id)
{
	printf("Distribute currently: service_id:%s,table:%d(0x%x)\n",service_id,PMT_table_id,PMT_table_id);
	/************Check IF DTB THIS TABLE**************/
	if( (!DTB_all_flag) && ( strcmp(service_id,DTB_stream_id)) )
		return;
	

	unsigned short pid=0;
	int cltid=0;
	int interval=0;

	unsigned short n=SPECIAL_STREAM_ID;
	char special_stream_id[16];
	sprintf(special_stream_id,"%d",n);

	/********** Get Distribute conditions**************/
	if(get_dtb_info(&pid,&cltid,&interval,baseid,serviceID,special_stream_id,PMT_table_id))
		return;

	/*****update packing flag*****/
	update_packing_flag(1,pid,PMT_table_id,baseid);
	
	/***********Create SECTIONS*****************/
	SEC_BUFFER_INFO sbi[256];
	memset(sbi,0,256*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	section_number=CreateOneTable_PMT(sbi,service_id);
	
	/************PACK************/

	pack_and_dtb(section_number, sbi, baseid,PMT_table_id,ds_server,ds_port,pid,cltid,interval,special_stream_id);

	/*****update packing flag*****/
	update_packing_flag(0,pid,PMT_table_id,baseid);
}


static DTBPMT()
{
	int i;
	int num=0;
	char data_service[50][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	
	//
	sprintf(sql,"SELECT id,service_id FROM xepg_sdt_extension WHERE id in \
				(select DISTINCT service_id FROM xepg_pmt_extension)");
	
	num=mysql_get_data(&mysql, sql,data_service);
	printf("service num:%d\n",num);
	
	for(i=0;i<num;i++)
	{
		DTBOneTable_PMT(data_service[i][0],data_service[i][1]);
	}
}



main(int argc,char **argv)
{

	read_argv_2(argc,argv);
	pack_init();
	DTBPMT();
	
}





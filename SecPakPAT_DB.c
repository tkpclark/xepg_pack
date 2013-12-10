#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"

#define MAX_PAT_SECTION_LEN 1024
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="PAT";
static char sql[512] = {0};
static unsigned int baseid=7;

char DTB_all_flag=0;
char DTB_stream_id[32];
unsigned char DTB_table_id=0;

char ds_server[32];
int ds_port=0;

static unsigned char PAT_table_id = 0x00;



static char foreigntype=1;

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

static char CreatePAS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number)
{
	printf("+++++section_number:%d+++++\n",section_number);
	p->buffer=malloc(MAX_PAT_SECTION_LEN);
	memset(p->buffer,0,MAX_PAT_SECTION_LEN);
	
	int i;
	unsigned int offs=0;
	int num=0,num1 = 0;
	unsigned short program_number  = 0;


	/******HEADER*******/
	NIT_HEADER pat_header;
	
	memcpy(pat_header.table_id,"00000000",sizeof(pat_header.table_id));
	memcpy(pat_header.section_syntax_indicator,"1",sizeof(pat_header.section_syntax_indicator));
	memcpy(pat_header.reserved_future_use0,"0",sizeof(pat_header.reserved_future_use0));
	memcpy(pat_header.reserved0,"11",sizeof(pat_header.reserved0));
	memcpy(pat_header.section_length,"000000000000",sizeof(pat_header.section_length));
	memcpy(pat_header.network_id,"0000000000000000",sizeof(pat_header.network_id));
	memcpy(pat_header.reserved1,"11",sizeof(pat_header.reserved1));
	memcpy(pat_header.version_number,"00000",sizeof(pat_header.version_number));
	memcpy(pat_header.current_next_indicator,"1",sizeof(pat_header.current_next_indicator));
	memcpy(pat_header.section_number,"00000000",sizeof(pat_header.section_number));
	memcpy(pat_header.last_section_number,"00000000",sizeof(pat_header.last_section_number));
	
	compose_asst((char*)&pat_header,sizeof(pat_header),"PAT HEADER",p->buffer+offs);
	offs+=sizeof(pat_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//transport_stream_id
	
	*(unsigned short *)(p->buffer+3)=0;  // transport_stream_id:0 ,before multiplexer,only one stream

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;

	//
	sprintf(sql,"SELECT service_id from xepg_sdt_extension where id in \
				(select DISTINCT service_id FROM xepg_pmt_extension)");
	char data_stream[200][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	num=mysql_get_data(&mysql, sql,data_stream);
	printf("num:%d\n",num);
	
	/******BODY*******/
	
	/* loop of programs */
	for(i=0;i<num;i++)
	{
		//program number
		SHORT2BYTE	program_number;
		program_number.shortbyte= atoi(data_stream[i][0]);
		*(unsigned char *)(p->buffer+offs) = program_number.byte.highbyte;	
		offs++;
		*(unsigned char *)(p->buffer+offs) = program_number.byte.lowbyte;
		offs++;

		sprintf(sql,"select pid from xepg_pid where extend=(select id from xepg_sdt_extension\
					where service_id=%d ) and baseid=8",atoi(data_stream[i][0]));
					
		char data_stream1[20][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
		num1=mysql_get_data(&mysql, sql,data_stream1);
		
		SHORT2BYTE	pid;
		pid.shortbyte = atoi(data_stream1[0][0]);
		*(unsigned char *)(p->buffer+offs) = pid.byte.highbyte;
		offs++;
		*(unsigned char *)(p->buffer+offs) = pid.byte.lowbyte;
		offs++;
		
	}

	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("section_length:%d \n",offs+4-3);
	print_HEX(p->buffer+1,2);
	printf("whole section:\n");
	print_HEX(p->buffer, offs+4-3);

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}



static int CreateOneTable_PAT(SEC_BUFFER_INFO *p_sbi)
{
	int ret;
	int num=0;
	unsigned char section_number=0;

	//version_number
	unsigned char version_number=0;
	version_number=0;//get_version_number(table_id, baseid, 0);
	
	while(1)
	{
		
		ret=CreatePAS(PAT_table_id,section_number,&p_sbi[section_number],version_number);
		section_number++;
		printf("\n\n");
		
		if(!ret)
			break;
		
	}
	/*
	sprintf(logbuf,"ver[%d]ts[0]tb[0x%x]sec[1]",version_number,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	PrintToFile(logfd, mdname, "ver[%d]ts[0]tb[0x%x]sec[1]",version_number,PAT_table_id);
	/* update last section number of every section */
	int i;
	for(i=0;i<section_number;i++)
	{
		//*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}

	return section_number;
}
static DTBOneTable_PAT(unsigned char table_id)//create actual NIT or other NIT
{

	unsigned short pid=0;
	int cltid=0;
	int interval=0;

	unsigned short n=SPECIAL_STREAM_ID;
	char special_stream_id[16];
	sprintf(special_stream_id,"%d",n);

	/********** Get Distribute conditions**************/
	if(get_dtb_info(&pid,&cltid,&interval,baseid,special_stream_id,special_stream_id,PAT_table_id))
		return;

	/*****update packing flag*****/
	update_packing_flag(1,pid,PAT_table_id,baseid);
	
	/***********Create SECTIONS*****************/
	SEC_BUFFER_INFO sbi[256];
	memset(sbi,0,256*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	section_number=CreateOneTable_PAT(sbi);
	
	/************PACK************/
	
	pack_and_dtb(section_number, sbi, baseid,PAT_table_id,ds_server,ds_port,pid,cltid,interval,special_stream_id);

	/*****update packing flag*****/
	update_packing_flag(0,pid,PAT_table_id,baseid);

}


static DTBPAT()
{
	int i;
	int stream_num=0;

	DTBOneTable_PAT(PAT_table_id);
}


main(int argc,char **argv)
{
	read_argv_2(argc, argv);
	pack_init();
	DTBPAT();
	
}




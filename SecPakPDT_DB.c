#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_PDT_SECTION_LEN 4096
//#define MAX_FTS_FILE_LEN MAX_PDT_SECTION_LEN-96

//#define PIC_COUNT 6

//static SECTION_INFO section_info;
//OCI_Connection* cn;
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="PDT";
static char sql[512];
static unsigned int baseid=52;
static unsigned char actual_PDT_table_id=0xAB;
static unsigned char all_PDT_table_id=0xAC;
static char DTB_all_flag=0;
static char DTB_stream_id[32];
static unsigned char DTB_table_id=0;
static unsigned short network_id;
static int networkID;
static char ds_server[32];
static int ds_port=0;

unsigned int printlevel = 0;

static char CreatePDS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,char *streamID,unsigned char version_number)
{
	printf("+++++section_number:%d+++++\n",section_number);
	p->buffer=malloc(MAX_PDT_SECTION_LEN);
	memset(p->buffer,0,MAX_PDT_SECTION_LEN);
	
	int i;
	unsigned int offs=0;
	unsigned int epgad_info_length_offs=0;
	unsigned int poster_info_length_offs=0;

	//unsigned int transport_stream_loop_length_offs=0;
	//unsigned int transport_descriptors_length_offs=0;

	/******HEADER*******/
	
	PDT_HEADER pdt_header;
	
	memcpy(pdt_header.table_id,"00000000",sizeof(pdt_header.table_id));
	memcpy(pdt_header.section_syntax_indicator,"1",sizeof(pdt_header.section_syntax_indicator));
	memcpy(pdt_header.reserved_future_use0,"1",sizeof(pdt_header.reserved_future_use0));
	memcpy(pdt_header.reserved0,"11",sizeof(pdt_header.reserved0));
	memcpy(pdt_header.section_length,"000000000000",sizeof(pdt_header.section_length));
	memcpy(pdt_header.trasport_stream_id,"0000000000000000",sizeof(pdt_header.trasport_stream_id));
	memcpy(pdt_header.reserved1,"11",sizeof(pdt_header.reserved1));
	memcpy(pdt_header.version_number,"00000",sizeof(pdt_header.version_number));
	memcpy(pdt_header.current_next_indicator,"1",sizeof(pdt_header.current_next_indicator));
	memcpy(pdt_header.section_number,"00000000",sizeof(pdt_header.section_number));
	memcpy(pdt_header.last_section_number,"00000000",sizeof(pdt_header.last_section_number));
	compose_asst((char*)&pdt_header,sizeof(pdt_header),"PDT HEADER",p->buffer+offs);
	offs+=sizeof(pdt_header)/8;
	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//stream_id
	if(table_id==actual_PDT_table_id)
		*(unsigned short *)(p->buffer+3)=htons(atoi(streamID));

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));
	
	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;
	

	/******BODY*******/
	//epgad_format_version	8
	*(unsigned char *)(p->buffer+offs)=0x1;
	offs++;
	//epgad_info_length	16
	epgad_info_length_offs=offs;
	offs+=2;
	/*
	For(i=0;i<N;i++){	
	
	   service_id	16
	   epgad_file_sub_id	16
	}
	*/
	int num_ad;
	//short serviceid_ad[]={0x0e21,0x0e01,0x00f1,0xea60,0x00c9,0x00cc,0x538};
	//short serviceid_ad[]={0x00f1,0xea60,0x00c9,0x00cc,0x538};
	//short serviceid_ad[]={3585,3617};
	//short serviceid_ad_sub[]={3585,3617};

	char data_service[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	if(table_id==0xab)//actual
		sprintf(sql,"SELECT t.serviceid,t.epgad_file_sub_id from xepg_pdt_extension t where serviceid IN (select id from xepg_sdt_extension where transport_stream_id=%d) and epgad_file_sub_id !='' and epgad_file_sub_id is not NULL",atoi(streamID));
	else if(table_id==0xac)//all
		sprintf(sql,"SELECT t.serviceid,t.epgad_file_sub_id from xepg_pdt_extension t where epgad_file_sub_id !='' and epgad_file_sub_id is not NULL ");
	num_ad=mysql_get_data(&mysql, sql, data_service);
	
	for(i=0;i<num_ad;i++)
	{
		//get service value
		sprintf(sql,"select t.service_id from xepg_sdt_extension t where t.id=%s",data_service[i][0]);
		mysql_get_data(&mysql, sql, data);
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data[0][0]));
		offs+=2;

		//get sub id
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data_service[i][1]));
		offs+=2;
	}
	
	//update epgad_info_length
	*(unsigned short *)(p->buffer+epgad_info_length_offs)=htons(offs-epgad_info_length_offs-2);
	
	//reserved	32
	*(unsigned int *)(p->buffer+offs)=0xFFFFFFFF;
	offs+=4;
	
	//poster_format_version	8
	*(unsigned char *)(p->buffer+offs)=0x1;
	offs++;

	//poster_info_length	16
	poster_info_length_offs=offs;
	offs+=2;
	

	/*
	For(i=0;i<N;i++){	
	   service_id	16
	   poster_file_sub_id	16
	   alone_price	16
	   show_period	8
	hide_period	8
	reserved	4
	descriptor_loop_length	12
	}
	*/
	//201//c9
		//3585//e01
		//3617//e21;
		//241//f1
		//60000//ea60
		//204//cc
		//1336//538
	int num_ce;
	if(table_id==0xab)//actual
		sprintf(sql,"SELECT t.serviceid,t.poster_file_sub_id ,t.alone_price,t.show_period,t.hide_period from xepg_pdt_extension t where serviceid IN (select id from xepg_sdt_extension where transport_stream_id=%d) and poster_file_sub_id !='' and poster_file_sub_id is not NULL",atoi(streamID));
	else if(table_id==0xac)//all
		sprintf(sql,"SELECT t.serviceid,t.poster_file_sub_id ,t.alone_price,t.show_period,t.hide_period from xepg_pdt_extension t where poster_file_sub_id !='' and poster_file_sub_id is not NULL ");
	num_ce=mysql_get_data(&mysql, sql, data_service);
	//short serviceid_ce[]={3585,3617};
	//short serviceid_ce_sub[]={1200,1201};
	//short serviceid_ce[]={4101,604};
	for(i=0;i<num_ce;i++)
	{
		//get service value
		sprintf(sql,"select t.service_id from xepg_sdt_extension t where t.id=%s",data_service[i][0]);
		mysql_get_data(&mysql, sql, data);
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data[0][0]));
		offs+=2;
		
		//poster_file_sub_id
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data_service[i][1]));
		//*(unsigned short *)(p->buffer+offs)=htons(serviceid_ce[i]);
		offs+=2;
		//alone_price
		*(unsigned short *)(p->buffer+offs)=htons(atoi(data_service[i][2]));
		offs+=2;
		//show period
		*(unsigned char *)(p->buffer+offs)=atoi(data_service[i][3]);
		offs++;
		//hide_period
		*(unsigned char *)(p->buffer+offs)=atoi(data_service[i][4]);
		offs++;
		//reserved and descriptor_loop_length
		*(unsigned short *)(p->buffer+offs)=htons(0x0);
		offs+=2;
	}
	//update poster_info_length_offs
	*(unsigned short *)(p->buffer+poster_info_length_offs)=htons(offs-poster_info_length_offs-2);
	
	
	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("section_length:%d ",offs+4-3);
	print_HEX(p->buffer+1,2);

	/*
	get_CRC(p->buffer,offs);
	//get_CRC1(file_sub_id,section_number,p->buffer+offs);
	
	*/
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
static int CreateOneTable_PDT(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id)
{
	int num=0;
	int ret;
	unsigned char section_number=0;
	
	//version_number
	unsigned char version_number=0;
	version_number=get_version_number(table_id, baseid, streamID);
	
	while(1)
	{
		
		ret=CreatePDS(table_id,section_number,&p_sbi[section_number],streamID,version_number);
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
	/*
	sprintf(logbuf,"ver[%d]ts[%s]tb[0x%x]sec[1]",version_number,stream_id,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	PrintToFile(logfd,mdname,"ver[%d]ts[%s]tb[0x%x]sec[1]",version_number,stream_id,table_id);
	return section_number;
}

static DTBOneTable_PDT(char *streamID,char *stream_id,unsigned char table_id)
{
	printf("Distribute currently: table:%d(0x%x) in stream:%s\n",table_id,table_id,stream_id);
	/************Check IF DTB THIS TABLE**************/
	if( (!DTB_all_flag) && ( strcmp(stream_id,DTB_stream_id) || (table_id!=DTB_table_id) ) )
		return;
	
	/***********SECTIONS*****************/
	

	int i=0,j=0;

	//printf("network num:%d\n",num);

	SEC_BUFFER_INFO sbi[256];
	memset(sbi,0,256*sizeof(SEC_BUFFER_INFO));
	int section_number=0;
	

	section_number=CreateOneTable_PDT(table_id,sbi,streamID,stream_id);


	
	pack_and_dtb(section_number, sbi, baseid, streamID,table_id,ds_server,ds_port);
}

static DTBPDT()
{
	int i;
	int stream_num=0;
	char data_stream[50][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	
	//get network_id
	get_network_id(&networkID,&network_id);
	
	
	sprintf(sql,"SELECT id, transport_stream_id from xepg_nit_extension where original_network_id=%d",networkID);
	stream_num=mysql_get_data(&mysql, sql,data_stream);
	printf("stream_num:%d\n",stream_num);
	for(i=0;i<stream_num;i++)
	{
		printf("########stream:%s#########\n",data_stream[i][0]);
		DTBOneTable_PDT(data_stream[i][0],data_stream[i][1],actual_PDT_table_id);
		DTBOneTable_PDT(data_stream[i][0],data_stream[i][1],all_PDT_table_id);
	}
}

void xepg_procquit(void)
{
	mysql_close(&mysql);
	printf("quiting...\n");
}
main(int argc,char **argv)
{
	strcpy(ds_server,argv[1]);
	ds_port=atoi(argv[2]);
	if( (argc==4) && !strcmp(argv[3],"all"))
	{
		DTB_all_flag=1;
		printf("Distribute all tables in all streams!\n");
	}
	else if(argc==5) 
	{
		strcpy(DTB_stream_id,argv[3]);
		DTB_table_id=(unsigned short)atoi(argv[4]);
		printf("Distribute table:%d(0x%x) in stream:%s\n",DTB_table_id,DTB_table_id,DTB_stream_id);
	}
	if(atexit(&xepg_procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	pack_init();
	DTBPDT();
}


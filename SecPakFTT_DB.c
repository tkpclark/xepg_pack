 #include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_FTT_SECTION_LEN 4096
#define MAX_FTS_FILE_LEN MAX_FTT_SECTION_LEN-96
#define MAX_PIC_SEC_NUM 200

#define MAX_PIC_NUM 50
#define MAX_SEC_NUM 512

static unsigned int baseid=53;
static unsigned char tableid_ad=0xAE;
static unsigned char tableid_ce=0xAD;
static char DTB_all_flag=0;
static char DTB_stream_id[32];
static char logbuf[2048];
static unsigned char DTB_table_id=0;
static unsigned short network_id;
static int networkID;

static char ds_server[32];
static int ds_port=0;

MYSQL mysql;
int logfd=0;
char mdname[]="FTT";
static char sql[512];
static unsigned int sbi_id=0;
static unsigned int pic_id=0;

unsigned int printlevel = 0;

typedef struct
{
	char name[128];
	unsigned int size;
	unsigned int file_sub_id;
	char stream[MAX_PIC_SEC_NUM*MAX_FTT_SECTION_LEN];
	unsigned int stream_len;
	char antifake[20];
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	unsigned short period;
	unsigned char style;
	//unsigned int offs;
}PIC_INFO;

static int get_file_stream(PIC_INFO *p_pic_info,char type)
{

	int fd;
	int offs=0;
	int header_len=0;
	int num;
	
	if(type==1)//epg ad
	{
		int pic_offset_offs=0;
		
		fd=open(p_pic_info->name,0);
		
		if(fd<0)
		{
			printf("open %s failed !\n",p_pic_info->name);
			return -1;
			
		}
		else
		{
			//get pic_size
			p_pic_info->size=get_file_size_f(p_pic_info->name);

			//magic 0xab1cd1ef
			*(unsigned char *)(p_pic_info->stream+offs)=0xab;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0x1c;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0xd1;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0xef;
			offs++;

			////format-verstion 8
			*(unsigned char *)(p_pic_info->stream+offs)=0x01;
			offs++;

			//data-version 8
			*(unsigned char *)(p_pic_info->stream+offs)=0x0;
			offs++;

			//layout 8
			*(unsigned char *)(p_pic_info->stream+offs)=p_pic_info->style;
			offs++;

			//period 16
			*(unsigned short *)(p_pic_info->stream+offs)=ntohs(p_pic_info->period);
			offs+=2;

			//x 16
			*(unsigned short *)(p_pic_info->stream+offs)=ntohs(p_pic_info->x);
			offs+=2;

			//y 16
			*(unsigned short *)(p_pic_info->stream+offs)=ntohs(p_pic_info->y);
			offs+=2;

			//width16
			*(unsigned short *)(p_pic_info->stream+offs)=ntohs(p_pic_info->width);
			offs+=2;

			//height 16
			*(unsigned short *)(p_pic_info->stream+offs)=ntohs(p_pic_info->height);
			offs+=2;

			//pic offset 32
			pic_offset_offs=offs;
			offs+=4;

			//picture size 32
			*(unsigned int *)(p_pic_info->stream+offs)=htonl(p_pic_info->size);
			offs+=4;

			//anti-fake 32
			HexString_to_Memory(p_pic_info->antifake, 8,p_pic_info->stream+offs);
			printf("binantifake: ");
			print_HEX(p_pic_info->stream+offs, 4);
			offs+=4;
			
			//action
			*(unsigned char *)(p_pic_info->stream+offs)=0x0;
			offs++;

			//url_length
			*(unsigned char *)(p_pic_info->stream+offs)=0x0;
			offs+=2;

			//url_string

			//update pic offset
			*(unsigned int *)(p_pic_info->stream+pic_offset_offs)=htonl(offs);
			header_len=offs;

			
			num=read(fd,p_pic_info->stream+offs,p_pic_info->size);
			if(num!=p_pic_info->size)
			{
				printf("read %s failed!num:%d|len:%d\n",p_pic_info->name,num,p_pic_info->size);
				return -1;
			}
			close(fd);
			
			offs+=p_pic_info->size;
		}


		p_pic_info->stream_len=offs;
		
		*(p_pic_info->stream+offs)=0;
		
	}
	else if(type==2)//poster
	{

		int pic_offset_offs=0;
		
		fd=open(p_pic_info->name,0);
		
		if(fd<0)
		{
			printf("open %s failed !\n",p_pic_info->name);
			return -1;
			
		}
		else
		{
			//get pic_size
			p_pic_info->size=get_file_size_f(p_pic_info->name);

			//magic 0xab1cd1ef
			*(unsigned char *)(p_pic_info->stream+offs)=0xab;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0x0c;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0xd0;
			offs++;
			*(unsigned char *)(p_pic_info->stream+offs)=0xef;
			offs++;

			//format-verstion 8
			*(unsigned char *)(p_pic_info->stream+offs)=0x01;
			offs++;

			//pic offset 16
			pic_offset_offs=offs;
			offs+=2;

			//picture size 32
			*(unsigned int *)(p_pic_info->stream+offs)=htonl(p_pic_info->size);
			offs+=4;

			//anti-fake 32
			HexString_to_Memory(p_pic_info->antifake, 4,p_pic_info->stream+offs);
			
			offs+=4;
			

			//x 16
			*(unsigned short *)(p_pic_info->stream+offs)=0;//htons(70);
			offs+=2;

			//y 16
			*(unsigned short *)(p_pic_info->stream+offs)=0;//htons(113);
			offs+=2;

			//action
			*(unsigned char *)(p_pic_info->stream+offs)=0x0;
			offs++;

			//url_length
			*(unsigned char *)(p_pic_info->stream+offs)=0x0;
			offs++;

			//url_string

			//update pic offset
			*(unsigned short *)(p_pic_info->stream+pic_offset_offs)=htons(offs);
			header_len=offs;

			
			
			
			num=read(fd,p_pic_info->stream+offs,p_pic_info->size);
			if(num!=p_pic_info->size)
			{
				printf("read %s failed!num:%d|len:%d\n",p_pic_info->name,num,p_pic_info->size);
				return -1;
			}
			close(fd);
			
			offs+=p_pic_info->size;
		}


		p_pic_info->stream_len=offs;
		
		*(p_pic_info->stream+offs)=0;

		//printf("pic %s stream:\n",p_pic_info->name);
		//print_HEX(p_pic_info->stream, p_pic_info->stream_len);
		
	}
	return 0;
	
}



static char CreateFTS(unsigned char table_id, PIC_INFO* p_pic_info,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number)
{
	//printf("Section start\n");
	printf("+++++section_number:%d+++++\n",section_number);
	p->buffer=malloc(MAX_FTT_SECTION_LEN);
	memset(p->buffer,0,MAX_FTT_SECTION_LEN);

	unsigned int offs=0;
	char sec_block_buffer[MAX_FTS_FILE_LEN];
	memset(sec_block_buffer,0,sizeof(sec_block_buffer));
	
	static unsigned int block_offset=0;
	unsigned short block_size=0;

	//unsigned int transport_stream_loop_length_offs=0;
	//unsigned int transport_descriptors_length_offs=0;

	/* header */
	
	FTT_HEADER ftt_header;
	
	
	memcpy(ftt_header.table_id,"00000000",sizeof(ftt_header.table_id));
	memcpy(ftt_header.section_syntax_indicator,"1",sizeof(ftt_header.section_syntax_indicator));
	memcpy(ftt_header.reserved_future_use0,"1",sizeof(ftt_header.reserved_future_use0));
	memcpy(ftt_header.reserved0,"11",sizeof(ftt_header.reserved0));
	memcpy(ftt_header.section_length,"000000000000",sizeof(ftt_header.section_length));
	memcpy(ftt_header.file_sub_id,"0000000000000000",sizeof(ftt_header.file_sub_id));
	memcpy(ftt_header.reserved1,"11",sizeof(ftt_header.reserved1));
	memcpy(ftt_header.version_number,"00000",sizeof(ftt_header.version_number));
	memcpy(ftt_header.current_next_indicator,"1",sizeof(ftt_header.current_next_indicator));
	memcpy(ftt_header.section_number,"00000000",sizeof(ftt_header.section_number));
	memcpy(ftt_header.last_section_number,"00000000",sizeof(ftt_header.last_section_number));

	
	compose_asst((char*)&ftt_header,sizeof(ftt_header),"FTT HEADER",p->buffer+offs);
	offs+=sizeof(ftt_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;
	printf("section_number:%d\n",section_number);

	//file_sub_id
	*(unsigned short *)(p->buffer+3)=htons(p_pic_info->file_sub_id);
	printf("file_sub_id:%d\n",p_pic_info->file_sub_id);
	
	/********header finished********/

	


	//reversed 4bits and descriptor length 12
	*(unsigned char*)(p->buffer+offs)=0xF0;
	offs++;
	*(unsigned char*)(p->buffer+offs)=0x00;
	offs++;
	

	//pic content can ocuppy MAX_FTT_SECTION_LEN-100 bytes
	
	//filesize 32
	*(unsigned int*)(p->buffer+offs)=htonl(p_pic_info->stream_len);
	printf("filesize:%d\n",p_pic_info->stream_len);
	offs+=4;
	

	//block_offset 32
	*(unsigned int*)(p->buffer+offs)=htonl(block_offset);
	printf("block_offset:%d\n",block_offset);
	offs+=4;
	
	
	//block_size 16
	//memset(sec_block_buffer,0,sizeof(sec_block_buffer));
	
	//block_size=read(fd,sec_block_buffer,MAX_FTS_FILE_LEN);
	if(p_pic_info->stream_len-block_offset >= MAX_FTS_FILE_LEN)
	{
		block_size=MAX_FTS_FILE_LEN;
	}
	else
	{
		block_size=p_pic_info->stream_len-block_offset;
	}
	*(unsigned short*)(p->buffer+offs)=htons(block_size);
	printf("block_size:%d\n",block_size);
	offs+=2;
	
	
	//printf("sec_block_buffer:\n");
	//print_HEX(sec_block_buffer, block_size);

	//block_data
	memcpy(p->buffer+offs,p_pic_info->stream+block_offset,block_size);
	offs+=block_size;

	block_offset+=block_size;

	

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

	//check this sub table is over or not
	if(block_offset==p_pic_info->stream_len)
	{
		block_offset=0;
		return 0;//over
	}
	else if(block_offset > p_pic_info->stream_len)
	{
		printf("block_offset > filesize!!!!\n");
		exit(0);
	}
	else
	{
		return 1;
	}
	
}
static mix_sbi(SEC_BUFFER_INFO *sbi,int pic_num,int table_section_number,int max_one_pic_section_number)
{
	int i,j,k,m=0;
	SEC_BUFFER_INFO sbi_m[MAX_PIC_SEC_NUM];

	memset(sbi_m,0,MAX_PIC_SEC_NUM*sizeof(SEC_BUFFER_INFO));


	printf("before mixing\n");
	for(i=0;i<table_section_number;i++)
	{
		printf("pic:%d,sec:%d,%d\n",sbi[i].pic_id,sbi[i].sec_id,sbi[i].len);
	}
	printf("mixing....\n");
	printf("picnum:%d,table_section_number:%d,max_one_pic_section_number:%d\n",pic_num,table_section_number,max_one_pic_section_number);
	for(i=0;i<max_one_pic_section_number;i++)//loop of sec_id
	{
		for(j=0;j<pic_num;j++)//loop of pic_id
		{
			for(k=0;k<table_section_number;k++)
			{
				//printf("%d :%d :%d:%d \n",i1,j1,sbi[k1].pic_id,sbi[k1].sec_id);
				if( (sbi[k].pic_id==j) && (sbi[k].sec_id==i))
				{
					//sbi_m[m].buffer=malloc(sbi[k].len);
					//memcpy(sbi_m[m].buffer,sbi[k].buffer,sbi[k].len);
					sbi_m[m].buffer=sbi[k].buffer;
					sbi_m[m].len=sbi[k].len;
					sbi_m[m].pic_id=sbi[k].pic_id;
					sbi_m[m].sec_id=sbi[k].sec_id;
					m++;
					//printf("M:%d\n",m);
				}
			}
		}
	}
	printf("m:%d\n",m);
	
	memcpy(sbi,sbi_m,MAX_PIC_SEC_NUM*sizeof(SEC_BUFFER_INFO));
	printf("after mixing\n");
	for(i=0;i<m;i++)
	{
		printf("pic:%d,sec:%d,%d\n",sbi[i].pic_id,sbi[i].sec_id,sbi[i].len);
	}

	
}
static CreateOneTable_FTT(unsigned char table_id,SEC_BUFFER_INFO *p_sbi, char *streamID,char *stream_id)
{
	//actual AD or other AD or actual AE or other AE,one of them
	printf("creating table 0x%x\n",table_id);
	
	PIC_INFO pic_info;
	char temp[128];
	unsigned int service_num=0;
	unsigned int num=0;
	int section_number=0;
	int table_section_number=0;
	int max_one_pic_section_number=0;
	int i,j;
	char ret=0;
	char data_service[100][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	char data_img[10][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];

	//get all services in this stream
	sprintf(sql,"SELECT id,service_id  from xepg_sdt_extension where transport_stream_id=%s",streamID);
	service_num=mysql_get_data(&mysql, sql,data_service);
	printf("service_num:%d\n",service_num);

	/*
	//get version_number of this table
	unsigned char version_number=0;
	version_number=get_version_number_task(table_id, baseid, streamID);
	*/
	
	//get ...
	char pic_table[32];
	char field_name[32];
	char table_type=0;

	int null_pic_section_flag=0;

	if(table_id==tableid_ad)
	{
		strcpy(pic_table,"xepg_ad");
		strcpy(field_name,"epgad_file_sub_id");
		table_type=1;
	}
	else if(table_id==tableid_ce)
	{
		strcpy(pic_table,"xepg_poster");
		strcpy(field_name,"poster_file_sub_id");
		table_type=2;
	}

	for(i=0;i<service_num;i++)// cirle of sub table 
	{
		printf("########serviceID:%d:%s########\n",i,data_service[i][1]);
		sprintf(logbuf,"ts[%s]tb[0x%x]sv[%s]",stream_id,table_id,data_service[i][1]);

		memset(&pic_info,0,sizeof(PIC_INFO));
		
		//////load pic info
		// there are 4 cases here ,
		// 1:there is a record in the db,and the pic exists; 			 deal:normal
		// 2:there is a record in the db,but the pic doesn't exist 		deal:only section header
		// 3:there is no record in the db  							deal:only section header
		// 4:there is an empty record after 7 seconds 				deal:only section header
		null_pic_section_flag=0;

		if(table_type==1)//only epg-ad do this
		{
			//check if it's empty record after 7 seconds
			sprintf(sql,"select *  from %s where dtbflag!=-1  and service_id=%s and (imgurl=NULL or imgurl is NULL or imgurl='')\
and(( NOW() + interval 7 second  between start_time and end_time )  or (NOW() >= start_time and NOW() < end_time ) )",pic_table,data_service[i][0]);
			num=mysql_get_data(&mysql, sql,data);
			if(num)
			{
				printf("next record is forbidden period, sending null section!\n");
				null_pic_section_flag=1;
				strcat(logbuf,"pic[fbd]");
				goto l;
			}
		}
		
		
		
		sprintf(sql,"select imgurl,antifake,xposition,yposition,width,height,showtime,style from %s where dtbflag!=-1  and service_id=%s and NOW() >= start_time and NOW() <  end_time",pic_table,data_service[i][0]);
		num=mysql_get_data(&mysql, sql,data_img);
		if(num)
		{
		
			//get pic_name 
			strcpy(pic_info.name,"/opt/tomcat/webapps/xepg");
			strcat(pic_info.name,data_img[0][0]);
			//printf("pic:%s\n",pic_info[j].name);

			//get antifake,x,y,width,height,period.stype
			strcpy(pic_info.antifake,data_img[0][1]);
			pic_info.x=atoi(data_img[0][2]);
			pic_info.y=atoi(data_img[0][3]);
			pic_info.width=atoi(data_img[0][4]);
			pic_info.height=atoi(data_img[0][5]);
			pic_info.period=atoi(data_img[0][6]);
			pic_info.style=atoi(data_img[0][7]);
			

		

			//get file_sub_id
			sprintf(sql,"select %s from xepg_pdt_extension where serviceid='%s'",field_name,data_service[i][0]);
			mysql_get_data(&mysql, sql,data);
			pic_info.file_sub_id=atoi(data[0][0]);
			
			//get file stream
			if(get_file_stream(&pic_info,table_type)==-1)
			{
				null_pic_section_flag=1;
				printf("get_file_stream failed !\n");
				strcat(logbuf,"pic[failed]");
				goto l;
			}


			sprintf(temp,"pic[%s]streamlen[%d]size[%d]anti[%s]x[%d]y[%d]width[%d]height[%d]period[%d]style[%d]",pic_info.name,pic_info.stream_len,pic_info.size,pic_info.antifake,pic_info.x,pic_info.y,pic_info.width,pic_info.height,pic_info.period,pic_info.style);
			strcat(logbuf,temp);
		
		}
		else
		{
			null_pic_section_flag=1;
			strcat(logbuf,"pic[empty]");
			goto l;
		}




	l:	
		if(null_pic_section_flag)//there are no rec in poster or ad table
		{
			pic_info.file_sub_id=htons(0);
			pic_info.size=0;
			pic_info.stream_len=0;
			printf("sending empty pic section!\n");
		}
		
		//creating sections of this sub table
		section_number=0;
		while(1)
		{
			p_sbi[table_section_number].pic_id=i;
			p_sbi[table_section_number].sec_id=section_number;
			ret=CreateFTS(table_id, &pic_info,section_number,&p_sbi[table_section_number],version_number);
			section_number++;
			table_section_number++;
			
			if(!ret)
				break;
		}

		/* update last section number of every section of this sub table */
		
		for(j=0;j<section_number;j++)
		{
			*(unsigned char *)(p_sbi[table_section_number-section_number+j].buffer+7)=section_number-1;
			get_CRC(p_sbi[table_section_number-section_number+j].buffer,p_sbi[table_section_number-section_number+j].len-4);
		}
		sprintf(temp,"sec[%d]",section_number);
		strcat(logbuf,temp);
		if(section_number > max_one_pic_section_number)
			max_one_pic_section_number=section_number;
		/*
		//update dtbflag
		sprintf(sql,"update %s set dtbflag=1 where id='%s'",pic_table,data[0][1]);
		mysql_query(&mysql,sql);
		*/
		
	
		////////////
		//strcat(logbuf,"\n");
		proclog(logfd,mdname,logbuf);
	}	
	mix_sbi(p_sbi,service_num,table_section_number,max_one_pic_section_number);
	printf("table_section_number:%d\n",table_section_number);
	return table_section_number;
	
	
}
static DTBOneTable_FTT(char *streamID,char *stream_id,unsigned char table_id)
{
	/*
		streamID:id,primary key in the table ,also using as foreign key
		stream_id:for user
		table_id:for identy it's actual or other
	*/
	int num=0;
	printf("Distribute currently: table:%d(0x%x) in stream:%s\n",table_id,table_id,stream_id);
	/************Check IF DTB THIS TABLE**************/
	if( (!DTB_all_flag) && ( strcmp(stream_id,DTB_stream_id) || (table_id!=DTB_table_id) ) )
		return;
	
	/***********Create SECTIONS*****************/

	
	unsigned short pid=0;
	int cltid=0;
	int interval=0;

	/********** Get Distribute conditions**************/
	if(get_dtb_info(&pid,&cltid,&interval,baseid,streamID,stream_id,table_id))
		return;

	/*****update packing flag*****/
	update_packing_flag(1,pid,table_id,baseid);
	
	/***********Create SECTIONS*****************/
	SEC_BUFFER_INFO sbi[MAX_PIC_SEC_NUM];
	memset(sbi,0,MAX_PIC_SEC_NUM*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	int i;
	for(i=0;i<MAX_PIC_SEC_NUM;i++)
	{
		sbi[i].pic_id=-1;
		sbi[i].sec_id=-1;
	}
	section_number=CreateOneTable_FTT(table_id,sbi,streamID,stream_id);
	
	/************PACK************/
	pack_and_dtb(section_number, sbi, baseid, stream_id,table_id,ds_server,ds_port,pid,cltid,interval);

	/*****update packing flag*****/
	update_packing_flag(0,pid,table_id,baseid);

	

}

static DTBFTT()
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
		printf("#########stream:%s#########\n",data_stream[i][0]);
		DTBOneTable_FTT(data_stream[i][0],data_stream[i][1],tableid_ad);//AE
		DTBOneTable_FTT(data_stream[i][0],data_stream[i][1],tableid_ce);//AD
		
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
		printf("Distribute table:%d(0x%x) in stream :%s to %s:%d\n",DTB_table_id,DTB_table_id,DTB_stream_id,ds_server,ds_port);
		
	}
	if(atexit(&xepg_procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	pack_init();
	DTBFTT();
}



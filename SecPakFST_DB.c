 #include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_FST_SECTION_LEN 4096
#define MAX_FTS_FILE_LEN MAX_FST_SECTION_LEN-96
#define MAX_PIC_SEC_NUM 400

#define MAX_PIC_NUM 50
#define MAX_SEC_NUM 512

static char logbuf[2048];
//static unsigned int baseid=54;
//static unsigned char table_id=0xAF;
//static unsigned char tableid_ce=0xAD;


TASK_INFO task_info;
MYSQL mysql;
int logfd=0;
char mdname[]="FST";
static char sql[512];

unsigned int printlevel = 0;

static char special_stream_id[16];

typedef struct
{
	char name[128];
	unsigned int size;
	unsigned int file_sub_id;
	char stream[MAX_PIC_SEC_NUM*MAX_FST_SECTION_LEN];
	unsigned int stream_len;
	unsigned int schedule_id;
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	unsigned short period;
	unsigned char style;
	unsigned int m_position;
	unsigned int m_len;
	unsigned short channelcode;
	//unsigned int offs;
}PIC_INFO;

static int get_file_stream(PIC_INFO *p_pic_info)
{

	int fd;
	int offs=0;
	int header_len=0;
	int num;

	int pic_offset_offs=0;

	fd=open(p_pic_info->name,0);

	if(fd<0)
	{
		printf("@@@打开图片 %s 失败 !\n",p_pic_info->name);
		return -1;
		
	}

	//get pic_size
	p_pic_info->size=get_file_size_f(p_pic_info->name);


	printf("@@@正在组织图片文件头所需信息......\n");
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
	*(unsigned char *)(p_pic_info->stream+offs)=0x02;
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

	//schedule_id 32(old antifake)
	*(unsigned int *)(p_pic_info->stream+offs)=htonl(p_pic_info->schedule_id);
	offs+=4;


	//m_position
	*(unsigned int *)(p_pic_info->stream+offs)=htonl(p_pic_info->m_position);
	offs+=4;

	//m_len
	*(unsigned int *)(p_pic_info->stream+offs)=htonl(p_pic_info->m_len);
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


	printf("@@@正在读取图片文件数据......\n");
	num=read(fd,p_pic_info->stream+offs,p_pic_info->size);
	if(num!=p_pic_info->size)
	{
		printf("@@@读取图片 %s 失败，实际读取大小与图片大小不符!实际读取:%d|实际长度:%d\n",p_pic_info->name,num,p_pic_info->size);
		return -1;
	}
	close(fd);

	offs+=p_pic_info->size;



	p_pic_info->stream_len=offs;

	*(p_pic_info->stream+offs)=0;


	return 0;

}

//creat one FTT section
static char CreateFSS(PIC_INFO* p_pic_info,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number)
{
	//printf("Section start\n");
	printf("@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_FST_SECTION_LEN);
	memset(p->buffer,0,MAX_FST_SECTION_LEN);

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

	
	compose_asst((char*)&ftt_header,sizeof(ftt_header),"FST HEADER",p->buffer+offs);
	offs+=sizeof(ftt_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=task_info.table_id;

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;
	printf("section_number:%d\n",section_number);


	
	//file_sub_id
	*(unsigned short *)(p->buffer+3)=htons(p_pic_info->file_sub_id);
	
	/********header finished********/

	


	//reversed 4bits and descriptor length 12
	*(unsigned char*)(p->buffer+offs)=0xF0;
	offs++;
	*(unsigned char*)(p->buffer+offs)=0x00;
	offs++;
	

	//pic content can ocuppy MAX_FTT_SECTION_LEN-100 bytes
	
	//filesize 32
	*(unsigned int*)(p->buffer+offs)=htonl(p_pic_info->stream_len);
	printf("@@@@@filesize:%d\n",p_pic_info->stream_len);
	offs+=4;
	

	//block_offset 32
	*(unsigned int*)(p->buffer+offs)=htonl(block_offset);
	printf("@@@@@block_offset:%d\n",block_offset);
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
	printf("@@@@@block_size:%d\n",block_size);
	offs+=2;
	
	
	//printf("sec_block_buffer:\n");
	//print_HEX(sec_block_buffer, block_size);

	//block_data
	memcpy(p->buffer+offs,p_pic_info->stream+block_offset,block_size);
	offs+=block_size;

	block_offset+=block_size;

	

	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("@@@@@section长度:%d\n",offs+4-3);
	printf("@@@@@打包完成!\n");
	//print_HEX(p->buffer+1,2);

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
		printf("@@@@@block_offset > filesize!!!!\n");
		exit(0);
	}
	else
	{
		return 1;
	}
	
}

//creat one ftt table include many sections

static int load_pic_info(PIC_INFO* p_pic_info,char *serviceID,char *service_id)
{
	//////load pic info
	// there are 4 cases here ,
	// 1:there is a record in the db,and the pic exists; 			 deal:normal
	// 2:there is a record in the db,but the pic doesn't exist 		deal:only section header
	// 3:there is no record in the db  							deal:only section header
	// 4:there is an empty record after 7 seconds 				deal:only section header

	printf("@@@正在从数据库中读取排期信息......\n");
	int num;
	char data_img[10][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	char pic_dir[256];
	read_config("pic_dir", pic_dir);

	sprintf(sql,"select imgurl,scheduleid,xposition,yposition,width,height,showtime,style,position,length,channelcode,id from xepg_ad where service_id=%s and NOW() >= start_time and NOW() <  end_time",serviceID);
	num=mysql_get_data(&mysql, sql,data_img);
	if(num)
	{



		//set dtbflag=1
		sprintf(sql,"update xepg_ad set dtbflag=1 where id='%s' ",data_img[0][11]);
		mysql_exec(&mysql, sql);


		
		//get pic_name 
		
		strcpy(p_pic_info->name,pic_dir);
		strcat(p_pic_info->name,data_img[0][0]);
		//printf("pic:%s\n",pic_info[j].name);

		//get antifake,x,y,width,height,period.stype
		p_pic_info->schedule_id=atoi(data_img[0][1]);
		p_pic_info->x=atoi(data_img[0][2]);
		p_pic_info->y=atoi(data_img[0][3]);
		p_pic_info->width=atoi(data_img[0][4]);
		p_pic_info->height=atoi(data_img[0][5]);
		p_pic_info->period=atoi(data_img[0][6]);
		p_pic_info->style=atoi(data_img[0][7]);
		p_pic_info->m_position=atoi(data_img[0][8]);
		p_pic_info->m_len=atoi(data_img[0][9]);
		p_pic_info->file_sub_id=atoi(data_img[0][10]);//channelcode

		
		//get file stream
		if(get_file_stream(p_pic_info)==-1)
		{
			return -1;
		}
		return 0;
	
	}
	else
	{
		printf("@@@没有当前的排期信息!\n");
		return -1;
	}
	


}
static int is_ad_time(char *serviceID)
{
	int num=0;

	printf("@@@正在检查7秒之后是否为禁播EPG时间......   \n");
	//check if it's empty record after 7 seconds
	sprintf(sql,"select *  from xepg_ad where service_id=%s and (imgurl=NULL or imgurl is NULL or imgurl='')\
and(( NOW() + interval 7 second  between start_time and end_time )  or (NOW() >= start_time and NOW() < end_time ) )",serviceID);
	num=mysql_get_data(&mysql, sql,data);
	if(num)
	{
		printf("@@@7秒之后为禁播时间，本次打空包!\n");
		return 1;
	}
	printf("@@@7秒之后可以播发EPG-AD，正常打包!\n");
	return 0;
}
static CreateOneTable_FST(SEC_BUFFER_INFO *p_sbi)
{

	PIC_INFO pic_info;
	unsigned int num=0;
	int section_number=0;
	int max_one_pic_section_number=0;
	int i,j;
	char ret=0;
	
	//get version_number of this table
	//unsigned char version_number=0;
	//version_number=get_version_number_task(table_id, baseid, serviceID);
	
	if(is_ad_time(task_info.extendID) ||load_pic_info(&pic_info, task_info.extendID,task_info.extend_id))
	{
		memset(&pic_info,0,sizeof(PIC_INFO));
	}

	//print picture infomation:
	unsigned short n=SPECIAL_STREAM_ID;
	sprintf(special_stream_id,"%d",n);
	sprintf(logbuf, "ts[%s]tb[0x%x(%d)]sv[%s]pic[%s]streamlen[%d]size[%d]sub[%d]scid[%d]x[%d]y[%d]width[%d]height[%d]period[%d]style[%d]channel[%d]positon[%d]len[%d]sec[%d]",
				special_stream_id,
				task_info.table_id,
				task_info.table_id,
				task_info.extend_id,
				pic_info.name,
				pic_info.stream_len,
				pic_info.size,
				pic_info.file_sub_id,
				pic_info.schedule_id,
				pic_info.x,
				pic_info.y,
				pic_info.width,
				pic_info.height,
				pic_info.period,
				pic_info.style,
				pic_info.file_sub_id,
				pic_info.m_position,
				pic_info.m_len,
				section_number);
	//proclog(logfd,mdname,logbuf);

	printf("@@@%s\n",logbuf);
	//creating sections of this sub table


	printf("@@@section打包开始......\n");
	section_number=0;
	while(1)
	{
		ret=CreateFSS(&pic_info,section_number,&p_sbi[section_number],task_info.version_number);
		section_number++;
		
		if(!ret)
			break;
	}

	/* update last section number of every section of this sub table */
	
	for(i=0;i<section_number;i++)
	{
		*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}

	//log
	/*
	sprintf(logbuf,"ts[0]tb[0x%x]sv[%s]pic[%s]streamlen[%d]size[%d]sub[%d]anti[%s]x[%d]y[%d]width[%d]height[%d]period[%d]style[%d]sec[%d]",
		table_id,service_id,pic_info.name,pic_info.stream_len,pic_info.size,pic_info.file_sub_id,pic_info.antifake,pic_info.x,pic_info.y,pic_info.width,pic_info.height,pic_info.period,pic_info.style,section_number);
	proclog(logfd,mdname,logbuf);
	*/



	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	

	return section_number;
	
}
static DTBOneTable_FST()
{


	//====update packing flag====
	update_packing_flag(1,task_info.task_pid,task_info.task_table_id);
	
	//====Get task info====
	if(get_task_info(&task_info)<0)
		exit(0);

	
	//====Create SECTIONS====
	SEC_BUFFER_INFO sbi[256];
	memset(sbi,0,256*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	section_number=CreateOneTable_FST(sbi);
	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);
	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);
	

}

main(int argc,char **argv)
{
	read_argv_1(argc,argv);

	pack_init();
	DTBOneTable_FST();
}



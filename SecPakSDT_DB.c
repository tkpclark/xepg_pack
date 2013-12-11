#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_SDT_SECTION_LEN 1024
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="SDT";
static char sql[512];


static char foreigntype=2;

TASK_INFO task_info;

unsigned int printlevel = 0;

static char CreateSDS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,char *streamID,char *stream_id,unsigned char version_number,DATA_LOOP *p_data_loop)
{
	printf("@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_SDT_SECTION_LEN);
	memset(p->buffer,0,MAX_SDT_SECTION_LEN);

	char buffer_tmp[MAX_SDT_SECTION_LEN];
	
	int i;
	unsigned int offs=0;
	unsigned int offs_tmp=0;
	unsigned int descriptors_loop_length_offs=0;
	int num=0;
	int len=0;


	/******HEADER*******/
	SDT_HEADER sdt_header;
	
	memcpy(sdt_header.table_id,"00000000",sizeof(sdt_header.table_id));
	memcpy(sdt_header.section_syntax_indicator,"1",sizeof(sdt_header.section_syntax_indicator));
	memcpy(sdt_header.reserved_future_use0,"1",sizeof(sdt_header.reserved_future_use0));
	memcpy(sdt_header.reserved0,"11",sizeof(sdt_header.reserved0));
	memcpy(sdt_header.section_length,"000000000000",sizeof(sdt_header.section_length));
	memcpy(sdt_header.transport_stream_id,"0000000000000000",sizeof(sdt_header.transport_stream_id));
	memcpy(sdt_header.reserved1,"11",sizeof(sdt_header.reserved1));
	memcpy(sdt_header.version_number,"00000",sizeof(sdt_header.version_number));
	memcpy(sdt_header.current_next_indiator,"1",sizeof(sdt_header.current_next_indiator));
	memcpy(sdt_header.section_number,"00000000",sizeof(sdt_header.section_number));
	memcpy(sdt_header.last_section_number,"00000000",sizeof(sdt_header.last_section_number));
	
	compose_asst((char*)&sdt_header,sizeof(sdt_header),"SDT HEADER",p->buffer+offs);
	offs+=sizeof(sdt_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//stream_id
	*(unsigned short *)(p->buffer+3)=htons(atoi(stream_id));

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));
	
	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;


	/******BODY*******/

	//original_network_id 16
	*(unsigned short *)(p->buffer+offs)=htons(task_info.network_id);
	offs+=2;

	//reserved_future_use
	*(unsigned char *)(p->buffer+offs)=0xFF;
	offs++;


	//get service info
	
	
	/* loop of services */
	
	while(1)
	{
		printf("offs:%d\n",offs);

		if(p_data_loop->num_all==p_data_loop->num_ps)
			break;
		memset(buffer_tmp,0,sizeof(buffer_tmp));
		offs_tmp=0;
		
		//transport_stream_id
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons((unsigned short)atoi(p_data_loop->data[p_data_loop->num_ps][1]));
		offs_tmp+=2;

		//reserved_future_use 6
		*(unsigned char*)(buffer_tmp+offs_tmp)=0xFF;
		
		//EIT_schedule_flag
		LET_BIT(*(unsigned char*)(buffer_tmp+offs_tmp),1,atoi(p_data_loop->data[p_data_loop->num_ps][4])); 

		//EIT_present_following_flag
		LET_BIT(*(unsigned char*)(buffer_tmp+offs_tmp),0,atoi(p_data_loop->data[p_data_loop->num_ps][5])); 

		offs_tmp++;

		//running_status
		*(unsigned char*)(buffer_tmp+offs_tmp)=(unsigned char)(atoi(p_data_loop->data[p_data_loop->num_ps][3]) << 5);

		//free_CA_mode
		LET_BIT(*(unsigned char*)(buffer_tmp+offs_tmp),4,atoi(p_data_loop->data[p_data_loop->num_ps][2])); 
		

		//record offs of descriptors_loop_length
		descriptors_loop_length_offs=offs_tmp;

		//make room for descriptors_loop_length
		offs_tmp+=2;

		//descriptors
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%s'  and t.layer='2' Order By descriptorid,groupid,seq",foreigntype,p_data_loop->data[p_data_loop->num_ps][0]);
		printf("@@@@@正在打包第二层描述符【ID:%s value:%s】......\n",p_data_loop->data[p_data_loop->num_ps][0],p_data_loop->data[p_data_loop->num_ps][1]);
		
		len=build_descriptors_memory(sql,buffer_tmp+offs_tmp);
		offs_tmp+=len;

		//update descriptors_loop_length_offs
		set_bits16(buffer_tmp+descriptors_loop_length_offs, offs_tmp-descriptors_loop_length_offs-2);

		
		//////check this section is full or not , or has finished all events
		if(offs+offs_tmp <= MAX_SDT_SECTION_LEN-50)
		{
			memcpy(p->buffer+offs,buffer_tmp,offs_tmp);
			offs+=offs_tmp;
			p_data_loop->num_ps++;
			printf("all:%d ,ps:%d section:%d\n",p_data_loop->num_all,p_data_loop->num_ps,section_number);
			if(p_data_loop->num_all==p_data_loop->num_ps)
			{
				break;
			}
				
		}
		else//this section is full
		{
			break;
		}
		
	}


	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("@@@@@section长度:%d\n",offs+4-3);
	printf("@@@@@打包完成!\n");

	

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}

static int CreateOneTable_SDT(SEC_BUFFER_INFO *p_sbi)
{
	int ret;
	int num=0;
	unsigned char section_number=0;
	int i,j;

	/*
	//version_number
	unsigned char version_number=0;
	version_number=get_version_number(table_id, baseid, CUR_streamID);
	*/
	
	DATA_LOOP data_loop;
	
	printf("@@@section打包开始......\n");
	if(task_info.table_id==0x42)//SDT actual
	{
		printf("@@@共有table_extension(transport stream)1个\n");
		sprintf(sql,"SELECT id, service_id,free_CA_mode,running_status,EIT_schedule_flag,EIT_present_following_flag from xepg_sdt_extension where transport_stream_id=%s",task_info.extendID);
		data_loop.num_all=mysql_get_data(&mysql, sql, data_loop.data);
		data_loop.num_ps=0;
		while(1)// for every transport stream(table_id extention)
		{
			
			ret=CreateSDS(task_info.table_id,section_number,&p_sbi[section_number],task_info.extendID,task_info.extend_id,task_info.version_number,&data_loop);
			section_number++;
			printf("\n\n");
			
			if(data_loop.num_all==data_loop.num_ps)//for making empy section
				break;
			
		}
		for(i=0;i<section_number;i++)
		{
			*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
			get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
			//print_HEX(sbi[i].buffer,sbi[i].len);

			//write_section(sbi,section_number);
		}
	}
	else if(task_info.table_id==0x46)//SDT other
	{
		
		int stream_num=0;
		unsigned char ts_section_number=0;
		char data_stream[50][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];

		//get all streams
		stream_num=get_all_streams(task_info.networkID,data_stream);

		printf("@@@共有tableid_extention(transport stream)%d个\n",stream_num-1);
		for(i=0;i<stream_num;i++)
		{
			if(!strcmp(data_stream[i][0],task_info.extendID))
				continue;

			printf("@@@transport stream:%s\n",data_stream[i][1]);
			
			sprintf(sql,"SELECT id, service_id,free_CA_mode,running_status,EIT_schedule_flag,EIT_present_following_flag from xepg_sdt_extension where transport_stream_id=%s",data_stream[i][0]);
			data_loop.num_all=mysql_get_data(&mysql, sql, data_loop.data);
			data_loop.num_ps=0;

			ts_section_number=0;
			while(1)// for every transport stream(table_id extention)
			{
				
				ret=CreateSDS(task_info.table_id,ts_section_number,&p_sbi[section_number],data_stream[i][0],data_stream[i][1],task_info.version_number,&data_loop);
				ts_section_number++;
				section_number++;
				printf("\n\n");
				
				if(data_loop.num_all==data_loop.num_ps)//for making empy section
					break;
				
			}


			//update last section number of this streams
			for(j=0;j<ts_section_number;j++)
			{
				*(unsigned char *)(p_sbi[section_number-ts_section_number+j].buffer+7)=ts_section_number-1;
				get_CRC(p_sbi[section_number-ts_section_number+j].buffer,p_sbi[section_number-ts_section_number+j].len-4);
				//print_HEX(sbi[i].buffer,sbi[i].len);

				//write_section(sbi,section_number);
			}
		}
	}
	else
	{
		printf("@@@错误!tableid:%d不属于SDT!\n",task_info.table_id);
		exit(0);
	}



	/*
	sprintf(logbuf,"ver[%d]ts[%s]tb[0x%x]",version_number,stream_id,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	//PrintToFile(logfd,mdname, "ver[%d]ts[%s]tb[0x%x]",version_number,stream_id,table_id);
	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	return section_number;
}
static DTBOneTable_SDT()
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
	section_number=CreateOneTable_SDT(sbi);
	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);

	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);
	
}

main(int argc,char **argv)
{

	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_SDT();
	
}



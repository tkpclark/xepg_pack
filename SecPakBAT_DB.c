#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_BAT_SECTION_LEN 1024
static char logbuf[2048];
MYSQL mysql;
int logfd=0;
char mdname[]="BAT";
static char sql[512];

static char foreigntype=4;

unsigned int printlevel = 0;

TASK_INFO task_info;

static char CreateBAS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,char *bouquetID,unsigned short bouquet_id,unsigned char version_number,DATA_LOOP *p_data_loop)
{
	printf("@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_BAT_SECTION_LEN);
	memset(p->buffer,0,MAX_BAT_SECTION_LEN);
	char buffer_tmp[MAX_BAT_SECTION_LEN];
	int i,g;
	unsigned int offs=0;
	unsigned int offs_tmp=0;
	unsigned int bouquet_descriptors_length_offs=0;
	unsigned int transport_stream_loop_length_offs=0;
	unsigned int transport_descriptors_length_offs=0;
	int num=0;


	/******HEADER*******/
	BAT_HEADER bat_header;
	
	memcpy(bat_header.table_id,"00000000",sizeof(bat_header.table_id));
	memcpy(bat_header.section_syntax_indicator,"1",sizeof(bat_header.section_syntax_indicator));
	memcpy(bat_header.reserved_future_use0,"1",sizeof(bat_header.reserved_future_use0));
	memcpy(bat_header.reserved0,"11",sizeof(bat_header.reserved0));
	memcpy(bat_header.section_length,"000000000000",sizeof(bat_header.section_length));
	memcpy(bat_header.bouquet_id,"0000000000000000",sizeof(bat_header.bouquet_id));
	memcpy(bat_header.reserved1,"11",sizeof(bat_header.reserved1));
	memcpy(bat_header.version_number,"00000",sizeof(bat_header.version_number));
	memcpy(bat_header.current_next_indicator,"1",sizeof(bat_header.current_next_indicator));
	memcpy(bat_header.section_number,"00000000",sizeof(bat_header.section_number));
	memcpy(bat_header.last_section_number,"00000000",sizeof(bat_header.last_section_number));
	//memcpy(bat_header.reserved_future_use1,"1111",sizeof(bat_header.reserved_future_use1));
	//memcpy(bat_header.bouquet_descriptors_length,"000000000000",sizeof(bat_header.bouquet_descriptors_length));
	
	compose_asst((char*)&bat_header,sizeof(bat_header),"BAT HEADER",p->buffer+offs);
	offs+=sizeof(bat_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//bouquet_id
	*(unsigned short *)(p->buffer+3)=htons(bouquet_id);

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	
	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;
	

	/******BODY*******/

	//record offs of bouquet_descriptors_length
	bouquet_descriptors_length_offs=offs;
	
	//reserved_future_use
	*(unsigned char *)(p->buffer+offs)=0xF0;
	//don't need offs plus here

	//make room for bouquet_descriptors_length
	offs+=2;
	unsigned short len=0;
	if(!section_number)//descriptor of layer 1 is only in the first section
	{
		printf("@@@@@正在打包第一层描述符......\n");
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%s'  and t.layer='1' Order By descriptorid,groupid,seq",foreigntype,bouquetID);
		len=build_descriptors_memory(sql,p->buffer+offs);//4 //foreigntype=4 means bat
		offs+=len;
	}
	else
	{
		len=0;
	}
	//update bouquet_descriptors_length
	set_bits16(p->buffer+bouquet_descriptors_length_offs, len);




	//record offs of transport_stream_loop_length
	transport_stream_loop_length_offs=offs;

	//reserved_future_use
	*(unsigned char*)(p->buffer+offs)=0xF0;
	
	//make room for transport_stream_loop_length
	offs+=2;

	//getting streams info
	//sprintf(sql,"select id,transport_stream_id from xepg_nit_extension where id IN (select transport_stream_id from xepg_sdt_extension where id IN (select service_id from xepg_bat_service where bouquet_id=%s))",bouquetID);

	/*
	sprintf(sql,"SELECT DISTINCT foreignextend FROM xepg_descriptor_relation_view where foreigntype='%x'  and foreignid='%s' and layer=2",foreigntype,bouquetID);
	num=mysql_get_data(&mysql, sql,data);
	*/

	
	char data_stream[2][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	
	/* loop of streams write until full*/
	while(1)
	{
		if(p_data_loop->num_all==p_data_loop->num_ps)
			break;
		memset(buffer_tmp,0,sizeof(buffer_tmp));
		offs_tmp=0;
		//transport_stream_id
		//get stream_id by streamID
		sprintf(sql,"select transport_stream_id from xepg_nit_extension where id=%s limit 1",p_data_loop->data[p_data_loop->num_ps][0]);
		mysql_get_data(&mysql, sql,data_stream);
	
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons((unsigned short)atoi(data_stream[0][0]));
		offs_tmp+=2;

		//original_network_id
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons(task_info.network_id);
		offs_tmp+=2;

		//record offs of transport_descriptors_length
		transport_descriptors_length_offs=offs_tmp;

		//reserved_future_use
		*(unsigned char*)(buffer_tmp+offs_tmp)=0xF0;

		//make room for bouquet_descriptors_length
		offs_tmp+=2;

		//transport_descriptors
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%s'  and t.layer='2'  and foreignextend=%s Order By descriptorid,groupid,seq",foreigntype,bouquetID,p_data_loop->data[p_data_loop->num_ps][0]);
		printf("@@@@@正在打包第二层描述符【ID:%s】......\n",p_data_loop->data[p_data_loop->num_ps][0],p_data_loop->data[p_data_loop->num_ps][1]);
		len=build_descriptors_memory(sql,buffer_tmp+offs_tmp);
		offs_tmp+=len;

		//update transport_descriptors_length_offs
		set_bits16(buffer_tmp+transport_descriptors_length_offs, offs_tmp-transport_descriptors_length_offs-2);



		//////check this section is full or not , or has finished all events
		if(offs+offs_tmp <= MAX_BAT_SECTION_LEN-10)
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

	//update transport_stream_loop_length
	printf("offs-transport_stream_loop_length:%d\n",offs-transport_stream_loop_length_offs);
	set_bits16(p->buffer+transport_stream_loop_length_offs, offs-transport_stream_loop_length_offs-2);


	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	//printf("section_length:%d ",offs+4-3);
	//print_HEX(p->buffer+1,2);
	printf("@@@@@section长度:%d\n",offs+4-3);
	printf("@@@@@打包完成!\n");

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}

static int CreateOneTable_BAT(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id)
{
	int num;
	int i,j;
	unsigned char section_number,all_section_number=0;
	printf("@@@section打包开始......\n");
	/*	
	//version_number
	unsigned char version_number=0;
	version_number=get_version_number(table_id, baseid, streamID);
	*/
	
	//get bouquet
	char data_bouquet_id[100][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	sprintf(sql,"select id,bouquet_id from xepg_bat_extension");
	num=mysql_get_data(&mysql, sql,data_bouquet_id);
	printf("@@@共有table_extension(banquet)%d个\n",num);
	DATA_LOOP data_loop;
	
	for(i=0;i<num;i++)//every sub table
	{

		printf("@@@banquet:%s(ID:%s)\n",data_bouquet_id[i][1],data_bouquet_id[i][0]);
		section_number=0;
		memset(&data_loop,0,sizeof(DATA_LOOP));
		
		sprintf(sql,"SELECT DISTINCT foreignextend FROM xepg_descriptor_relation_view where foreigntype='%x'  and foreignid='%s' and layer=2",foreigntype,data_bouquet_id[i][0]);
		data_loop.num_all=mysql_get_data(&mysql, sql, data_loop.data);
		
		while(1)
		{
			
			CreateBAS(table_id,section_number,&p_sbi[all_section_number],data_bouquet_id[i][0],(unsigned short)atoi(data_bouquet_id[i][1]),task_info.version_number,&data_loop);
			section_number++;
			all_section_number++;
			printf("\n\n");
			if(data_loop.num_all==data_loop.num_ps)//for making empy section
				break;
		}
		
		
		//PrintToFile(logfd, mdname, "ver[%d]ts[%s]tb[0x%x]bqt[%s]sec[%d]",version_number,stream_id,table_id,data_bouquet_id[i][0],section_number);
		/* update last section number of every section */

		for(j=0;j<section_number;j++)
		{
			*(unsigned char *)(p_sbi[all_section_number-section_number+j].buffer+7)=section_number-1;
			get_CRC(p_sbi[all_section_number-section_number+j].buffer,p_sbi[all_section_number-section_number+j].len-4);
			//print_HEX(sbi[i].buffer,sbi[i].len);

			//write_section(sbi,section_number);
		}
		
	}

	

	

	return all_section_number;
}
static DTBOneTable_BAT()//create actual BAT or other BAT
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
	section_number=CreateOneTable_BAT(task_info.table_id,sbi,task_info.extendID,task_info.extend_id);
	
	//====PACK====
	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);

}

main(int argc,char **argv)
{
	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_BAT();
	
}


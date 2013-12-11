#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_NIT_SECTION_LEN 1024
static char logbuf[2048];
MYSQL mysql;
int logfd=0;
char mdname[]="NIT";
static char sql[512];


TASK_INFO task_info;


static char foreigntype=1;

unsigned int	printlevel = 0;

static int get_one_MNIT_streams(char data_stream[][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN])
{
	int stream_num=0;
	printf("@@@正在读取MNIT的流信息......\n");
	//sprintf(sql,"SELECT id, transport_stream_id from xepg_nit_extension where transport_stream_id!=%d and original_network_id=%d",special_stream_id,networkID);
	sprintf(sql,"SELECT t1.transport_stream_id,t2.transport_stream_id FROM xepg_nit_ts t1,xepg_nit_extension t2 where t1.transport_stream_id=t2.id and t1.baseid=%d",task_info.baseid);
	stream_num=mysql_get_data(&mysql, sql,data_stream);
	printf("@@@本次MNIT打包共有流%d个\n",stream_num);
	if(!stream_num)
	{
		proclog(logfd,mdname,"no stream!\n");

		printf("@@@没有流信息!\n");
		exit(0);
		
	}
	return stream_num;
}
static char CreateNIS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned char version_number,DATA_LOOP *p_data_loop)
{
	printf("@@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_NIT_SECTION_LEN);
	memset(p->buffer,0,MAX_NIT_SECTION_LEN);

	char buffer_tmp[MAX_NIT_SECTION_LEN];
	
	int i;
	unsigned int offs=0;
	unsigned int offs_tmp=0;
	unsigned int network_descriptors_length_offs=0;
	unsigned int transport_stream_loop_length_offs=0;
	unsigned int transport_descriptors_length_offs=0;
	int num=0;


	/******HEADER*******/
	NIT_HEADER nit_header;
	
	memcpy(nit_header.table_id,"00000000",sizeof(nit_header.table_id));
	memcpy(nit_header.section_syntax_indicator,"1",sizeof(nit_header.section_syntax_indicator));
	memcpy(nit_header.reserved_future_use0,"1",sizeof(nit_header.reserved_future_use0));
	memcpy(nit_header.reserved0,"11",sizeof(nit_header.reserved0));
	memcpy(nit_header.section_length,"000000000000",sizeof(nit_header.section_length));
	memcpy(nit_header.network_id,"0000000000000000",sizeof(nit_header.network_id));
	memcpy(nit_header.reserved1,"11",sizeof(nit_header.reserved1));
	memcpy(nit_header.version_number,"00000",sizeof(nit_header.version_number));
	memcpy(nit_header.current_next_indicator,"1",sizeof(nit_header.current_next_indicator));
	memcpy(nit_header.section_number,"00000000",sizeof(nit_header.section_number));
	memcpy(nit_header.last_section_number,"00000000",sizeof(nit_header.last_section_number));
	//memcpy(nit_header.reserved_future_use1,"1111",sizeof(nit_header.reserved_future_use1));
	//memcpy(nit_header.network_descriptors_length,"000000000000",sizeof(nit_header.network_descriptors_length));
	
	compose_asst((char*)&nit_header,sizeof(nit_header),"NIT HEADER",p->buffer+offs);
	offs+=sizeof(nit_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//network_id
	*(unsigned short *)(p->buffer+3)=htons(task_info.network_id);

	//version_number
	printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;


	/******BODY*******/

	//record offs of network_descriptors_length
	network_descriptors_length_offs=offs;
	
	//reserved_future_use
	*(unsigned char *)(p->buffer+offs)=0xF0;
	//don't need offs plus here

	//make room for network_descriptors_length
	offs+=2;

	//network_descriptors
	unsigned short len=0;
	printf("@@@@@正在打包第一层描述符......\n");

	if(task_info.base_type==1)//NIT
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%d'  and t.layer='1' Order By descriptorid,groupid,seq",foreigntype,task_info.networkID);
	else if(task_info.base_type==100)//MNIT
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where  ( (foreigntype=1 and status=0  ) or (foreigntype=%d ) ) and t.foreignid='%d' and layer='1'  Order By descriptorid,groupid,seq",task_info.baseid,task_info.networkID);
	len=build_descriptors_memory(sql,p->buffer+offs);
	offs+=len;

	//update network_descriptors_length
	set_bits16(p->buffer+network_descriptors_length_offs, len);




	//record offs of transport_stream_loop_length
	transport_stream_loop_length_offs=offs;

	//reserved_future_use
	*(unsigned char*)(p->buffer+offs)=0xF0;
	
	//make room for network_descriptors_length
	offs+=2;
	
	
	/* loop of streams write until full*/
	while(1)
	{
		if(p_data_loop->num_all==p_data_loop->num_ps)
			break;
		memset(buffer_tmp,0,sizeof(buffer_tmp));
		offs_tmp=0;
		//transport_stream_id
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons((unsigned short)atoi(p_data_loop->data[p_data_loop->num_ps][1]));
		offs_tmp+=2;

		//original_network_id
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons(task_info.network_id);
		offs_tmp+=2;

		//record offs of transport_descriptors_length
		transport_descriptors_length_offs=offs_tmp;

		//reserved_future_use
		*(unsigned char*)(buffer_tmp+offs_tmp)=0xF0;

		//make room for network_descriptors_length
		offs_tmp+=2;

		//transport_descriptors
		printf("@@@@@正在打包第二层描述符【ID:%s value:%s】......\n",p_data_loop->data[p_data_loop->num_ps][0],p_data_loop->data[p_data_loop->num_ps][1]);
		
		
		
		if(task_info.base_type==1)//NIT
			sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%s'  and t.layer='2' Order By descriptorid,groupid,seq",foreigntype,p_data_loop->data[p_data_loop->num_ps][0]);
		else if(task_info.base_type==100)//MNIT
			sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where ( (foreigntype=1 and status=0  ) or (foreigntype=%d ) ) and t.foreignid='%s' and layer='2' Order By descriptorid,groupid,seq", task_info.baseid,p_data_loop->data[p_data_loop->num_ps][0]);
		len=build_descriptors_memory(sql,buffer_tmp+offs_tmp);
		offs_tmp+=len;

		//update transport_descriptors_length_offs
		set_bits16(buffer_tmp+transport_descriptors_length_offs, offs_tmp-transport_descriptors_length_offs-2);



		//////check this section is full or not , or has finished all events
		if(offs+offs_tmp <= MAX_NIT_SECTION_LEN-50)
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
	printf("@@@@@section长度:%d\n",offs+4-3);
	printf("@@@@@打包完成!\n");
	//print_HEX(p->buffer+1,2);

	

	offs+=4;//for CRC
	p->len=offs;

	return 0;

	
}

static int CreateOneTable_NIT(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id)
{
	int ret;
	int num=0;
	unsigned char section_number=0;

	/*
	//version_number
	unsigned char version_number=0;
	version_number=get_version_number(table_id, baseid, streamID);
	*/
	printf("@@@section打包开始......\n");
	DATA_LOOP data_loop;

	

	//get all streams
	if(task_info.base_type==1)//NIT
		data_loop.num_all=get_all_streams(task_info.networkID,data_loop.data);
	else if(task_info.base_type==100)//MNIT
		data_loop.num_all=get_one_MNIT_streams(data_loop.data);	

	printf("@@@共有table_extension(network)1个\n");
	data_loop.num_ps=0;
	while(1)
	{
		
		CreateNIS(table_id,section_number,&p_sbi[section_number],task_info.version_number,&data_loop);
		section_number++;
		printf("\n\n");
		
		if(data_loop.num_all==data_loop.num_ps)//for making empy section
			break;

	}
	/*
	sprintf(logbuf,"ver[%d]ts[%s]tb[0x%x]sec[%d]",version_number,stream_id,table_id,section_number);
	proclog(logfd, mdname, logbuf);
	*/
	//PrintToFile(logfd,  mdname, "ver[%d]ts[%s]tb[0x%x]sec[%d]",version_number,stream_id,table_id,section_number);
	/* update last section number of every section */
	int i;
	for(i=0;i<section_number;i++)
	{
		*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}


	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	return section_number;
}
static DTBOneTable_NIT()
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
	section_number=CreateOneTable_NIT(task_info.table_id,sbi,task_info.extendID,task_info.extend_id);
	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);
	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);
	
	
}

main(int argc,char **argv)
{

	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_NIT();
	
}


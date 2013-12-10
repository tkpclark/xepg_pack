#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_EIT_SECTION_LEN 4096
#define MAX_EIT_SECTION_NUM 
MYSQL mysql;
int logfd=0;
char mdname[]="EIT";
static char sql[512];
static char logbuf[2048];
static unsigned int baseid=3;
static unsigned char actual_EIT_PF_table_id=0x4e;
static unsigned char other_EIT_PF_table_id=0x4f;
static unsigned char actual_EIT_SC_table_id=0x50;
static unsigned char other_EIT_SC_table_id=0x60;

unsigned short special_stream_id=SPECIAL_STREAM_ID;

static char foreigntype=3;

unsigned int printlevel = 0;
TASK_INFO task_info;
//static unsigned short glb_event_id=0;

static char CreateEIS_PF(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned short stream_id,char *serviceID,unsigned short service_id,unsigned char version_number)
{
	//printf("	---section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_EIT_SECTION_LEN);
	memset(p->buffer,0,MAX_EIT_SECTION_LEN);
	
	int i;
	char tmStr[24];
	unsigned int offs=0;
	unsigned int descriptors_loop_length_offs=0;
	int num=0;
	int len=0;


	/******HEADER*******/
	EIT_HEADER eit_header;
	
	memcpy(eit_header.table_id,"00000000",sizeof(eit_header.table_id));
	memcpy(eit_header.section_syntax_indicator,"1",sizeof(eit_header.section_syntax_indicator));
	memcpy(eit_header.reserved_future_use0,"1",sizeof(eit_header.reserved_future_use0));
	memcpy(eit_header.reserved0,"11",sizeof(eit_header.reserved0));
	memcpy(eit_header.section_length,"000000000000",sizeof(eit_header.section_length));
	memcpy(eit_header.service_id,"0000000000000000",sizeof(eit_header.service_id));
	memcpy(eit_header.reserved1,"11",sizeof(eit_header.reserved1));
	memcpy(eit_header.version_number,"00000",sizeof(eit_header.version_number));
	memcpy(eit_header.current_next_indicator,"1",sizeof(eit_header.current_next_indicator));
	memcpy(eit_header.section_number,"00000000",sizeof(eit_header.section_number));
	memcpy(eit_header.last_section_number,"00000000",sizeof(eit_header.last_section_number));
	
	compose_asst((char*)&eit_header,sizeof(eit_header),"EIT HEADER",p->buffer+offs);
	offs+=sizeof(eit_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//service_id
	*(unsigned short *)(p->buffer+3)=htons(service_id);

	//version_number
	//printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	//printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;

	//last_section_number
	*(unsigned char *)(p->buffer+7)=1;


	/******BODY*******/

	
	//stream_id
	/*
	sprintf(sql,"select transport_stream_id from xepg_sdt_extension where service_id=%d",serviceID);
	mysql_get_data(&mysql, sql,data);
	*(unsigned short *)(p->buffer+offs)=htons(atoi(data[0][0]));
	*/
	*(unsigned short *)(p->buffer+offs)=htons(stream_id);
	offs+=2;
	
	//original_network_id 16
	*(unsigned short *)(p->buffer+offs)=htons(task_info.network_id);
	offs+=2;

	//segment_last_section_number
	*(unsigned char *)(p->buffer+offs)=1;
	offs++;

	//last_table_id
	*(unsigned char *)(p->buffer+offs)=table_id;
	offs++;


	//get event info
	if(section_number==0)//actual
	{
		sprintf(sql,"select id,start_time-interval 8 hour,duration,running_status,free_CA_mode,event_id from xepg_eit_extension where service_id=%s and  NOW() >=  start_time and NOW() < start_time + interval HOUR(duration ) HOUR + interval MINUTE(duration ) MINUTE + interval SECOND(duration ) SECOND ",serviceID);
	}
	else if(section_number==1)
	{
		sprintf(sql,"select id,start_time-interval 8 hour,duration,running_status,free_CA_mode,event_id from xepg_eit_extension where service_id=%s and start_time > NOW() order by start_time limit 1",serviceID);
	}
	else
	{
		printf("section_number error:%d\n",section_number);
	}
	num=mysql_get_data(&mysql, sql,data);
	
	
	/* loop of events */
	unsigned short event_id;
	//for(i=0;i<1;i++)//only 1 is curring event
	
	if(num)
	{

		if(section_number==0)//only p do update
		{
			//set dtbflag=1
			sprintf(sql,"update xepg_eit_extension set dtbflag=1 where id='%s' ",data[0][0]);
			mysql_exec(&mysql, sql);
		}

				
		i=0;
		//event_id
		event_id=(unsigned short)(atoi(data[i][5]));
		//event_id=atoi(data[i][0]);
		*(unsigned short*)(p->buffer+offs)=htons(event_id);
		//printf("pf event id:%d\n",event_id);
		offs+=2;

		//MJD 16
		*(unsigned short*)(p->buffer+offs)=htons(CalcMJDByDate(data[i][1]));
		//printf("MJD: ");
		//print_HEX(p->buffer+offs,2);
		offs+=2;


		//time BCD 24
		strcpy(tmStr,data[i][1]+11);
		trimTimeStr(tmStr);
		HexString_to_Memory(tmStr,6, p->buffer+offs);
		offs+=3;

		//duration 24
		strncpy(tmStr,data[i][2],8);
		trimTimeStr(tmStr);
		HexString_to_Memory(tmStr, 6, p->buffer+offs);
		offs+=3;

		//printf("Hex:");
		//print_HEX(p->buffer+offs-8, 8);
		
		//running_status
		*(unsigned char*)(p->buffer+offs)=(unsigned char)(atoi(data[i][3]) << 5);

		//free_CA_mode
		LET_BIT(*(unsigned char*)(p->buffer+offs),4,atoi(data[i][4])); 
		

		//record offs of descriptors_loop_length
		descriptors_loop_length_offs=offs;

		//make room for descriptors_loop_length
		offs+=2;

		//descriptors
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid='%s'  and t.layer='2' Order By descriptorid,groupid,seq",foreigntype,data[i][0]);
		//printf("			---正在打包第二层描述符【ID:%s value:%s】......\n",data[i][0],data[i][1]);
		len=build_descriptors_memory(sql,p->buffer+offs);
		offs+=len;

		//update descriptors_loop_length_offs
		set_bits16(p->buffer+descriptors_loop_length_offs, offs-descriptors_loop_length_offs-2);
		
	}

	

	
		/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	//printf("			---section长度:%d\n",offs+4-3);
	//printf("			---打包完成!\n");
	//print_HEX(p->buffer+1,2);

	

	

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}

static char CreateEIS_SC(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p,unsigned short service_id,unsigned short stream_id,unsigned char version_number,DATA_LOOP *p_data_loop)
{
	//printf("			---section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_EIT_SECTION_LEN);
	if(p->buffer==NULL)
	{
		printf("@@@@@@没有足够的内存进行分配!");
		exit(0);
	}
	memset(p->buffer,0,MAX_EIT_SECTION_LEN);

	char buffer_tmp[MAX_EIT_SECTION_LEN];
	
	int i;
	char tmStr[24];
	unsigned int offs=0;
	unsigned int offs_tmp=0;
	unsigned int descriptors_loop_length_offs=0;
	
	
	int num=0;
	int len=0;


	/******HEADER*******/
	EIT_HEADER eit_header;
	
	memcpy(eit_header.table_id,"00000000",sizeof(eit_header.table_id));
	memcpy(eit_header.section_syntax_indicator,"1",sizeof(eit_header.section_syntax_indicator));
	memcpy(eit_header.reserved_future_use0,"1",sizeof(eit_header.reserved_future_use0));
	memcpy(eit_header.reserved0,"11",sizeof(eit_header.reserved0));
	memcpy(eit_header.section_length,"000000000000",sizeof(eit_header.section_length));
	memcpy(eit_header.service_id,"0000000000000000",sizeof(eit_header.service_id));
	memcpy(eit_header.reserved1,"11",sizeof(eit_header.reserved1));
	memcpy(eit_header.version_number,"00000",sizeof(eit_header.version_number));
	memcpy(eit_header.current_next_indicator,"1",sizeof(eit_header.current_next_indicator));
	memcpy(eit_header.section_number,"00000000",sizeof(eit_header.section_number));
	memcpy(eit_header.last_section_number,"00000000",sizeof(eit_header.last_section_number));
	
	compose_asst((char*)&eit_header,sizeof(eit_header),"EIT HEADER",p->buffer+offs);
	offs+=sizeof(eit_header)/8;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;

	//service_id
	*(unsigned short *)(p->buffer+3)=htons(service_id);

	//version_number
	//printf("version number:%d\n",version_number);
	*(unsigned char *)(p->buffer+5)= version_number << 1 | *(unsigned char *)(p->buffer+5);
	//printf("version_number HEX:%X\n",*(unsigned char *)(p->buffer+5));

	//section_number
	*(unsigned char *)(p->buffer+6)=section_number;


	/******BODY*******/

	
	//stream_id
	//sprintf(sql,"select transport_stream_id from xepg_sdt_extension where service_id=%d",service_id);
	//mysql_get_data(&mysql, sql,data);
	*(unsigned short *)(p->buffer+offs)=htons(stream_id);
	offs+=2;
	
	//original_network_id 16
	*(unsigned short *)(p->buffer+offs)=htons(task_info.network_id);
	offs+=2;

	//segment_last_section_number
	//*(unsigned char *)(p->buffer+offs)=1;
	offs++;

	//last_table_id
	//*(unsigned char *)(p->buffer+offs)=table_id;
	offs++;

	//get event info
	
	
	
	/* loop of events */
	unsigned short event_id;
	
	while(1)
	{
		if(p_data_loop->num_all==p_data_loop->num_ps)
			break;
		memset(buffer_tmp,0,sizeof(buffer_tmp));
		offs_tmp=0;
		//event_id
		event_id=(unsigned short)(atoi(p_data_loop->data[p_data_loop->num_ps][6]));
		//event_id=atoi(p_data_loop->data[p_data_loop->num_ps][0]);
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons(event_id);
		//printf("sc event id:%d\n",event_id);
		offs_tmp+=2;

		//MJD 16
		*(unsigned short*)(buffer_tmp+offs_tmp)=htons(CalcMJDByDate(p_data_loop->data[p_data_loop->num_ps][1]));
		//printf("MJD: ");
		//print_HEX(buffer_tmp+offs_tmp,2);
		offs_tmp+=2;


		//time BCD 24
		strcpy(tmStr,p_data_loop->data[p_data_loop->num_ps][1]+11);
		trimTimeStr(tmStr);
		HexString_to_Memory(tmStr,6, buffer_tmp+offs_tmp);
		offs_tmp+=3;

		//duration 24
		strncpy(tmStr,p_data_loop->data[p_data_loop->num_ps][2],8);
		trimTimeStr(tmStr);
		HexString_to_Memory(tmStr, 6, buffer_tmp+offs_tmp);
		offs_tmp+=3;

		//printf("Hex:");
		//print_HEX(buffer_tmp+offs_tmp-8, 8);
		
		//running_status
		*(unsigned char*)(buffer_tmp+offs_tmp)=(unsigned char)(atoi(p_data_loop->data[p_data_loop->num_ps][3]) << 5);

		//free_CA_mode
		LET_BIT(*(unsigned char*)(buffer_tmp+offs_tmp),4,atoi(p_data_loop->data[p_data_loop->num_ps][4])); 

		//printf("running status and free CA mode: ");
		//print_HEX(buffer_tmp+offs_tmp, 1);
		

		//record offs of descriptors_loop_length
		descriptors_loop_length_offs=offs_tmp;

		//make room for descriptors_loop_length
		offs_tmp+=2;

		//descriptors
		sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where descriptorid!=195 and descriptorid!=196 and t.foreigntype='%x' and t.foreignid='%s'  and t.layer='2' Order By descriptorid,groupid,seq",foreigntype,p_data_loop->data[p_data_loop->num_ps][0]);
		//printf("			---正在打包第二层描述符【ID:%s value:%s】......\n",p_data_loop->data[p_data_loop->num_ps][0],p_data_loop->data[p_data_loop->num_ps][1]);
		len=build_descriptors_memory(sql,buffer_tmp+offs_tmp);
		offs_tmp+=len;

		/*
		//short event descriptor

		//descriptor_tag
		*(unsigned char*)(buffer_tmp+offs_tmp)=0x4d;

		//descriptor length

		//record offs of descriptors_loop_length
		descriptors_length_offs=offs_tmp;

		//make room for descriptors_length
		offs_tmp++;

		//ISO 639_2_language_code
		strcpy(buffer_tmp+offs_tmp,"chi");
		offs_tmp+=3;

		//event_name_length
		printf("event name:%s len:%d\n",p_data_loop->data[p_data_loop->num_ps][5],(unsigned char)strlen(p_data_loop->data[p_data_loop->num_ps][5]));
		*(unsigned char*)(buffer_tmp+offs_tmp)=(unsigned char)strlen(p_data_loop->data[p_data_loop->num_ps][5]);
		offs_tmp++;

		//event_name
		strcpy(buffer_tmp+offs_tmp,p_data_loop->data[p_data_loop->num_ps][5]);
		offs_tmp+=(unsigned char)strlen(p_data_loop->data[p_data_loop->num_ps][5]);
		
		//update descriptors_length_offs
		*(unsigned char*)(buffer_tmp+descriptors_length_offs)=offs_tmp-descriptors_length_offs-1;
		printf("short event descriptor length:%d\n",offs_tmp-descriptors_length_offs-1);

		*/
		//update descriptors_loop_length_offs
		set_bits16(buffer_tmp+descriptors_loop_length_offs, offs_tmp-descriptors_loop_length_offs-2);
		//printf("descriptors_loop_length:%d\n",offs_tmp-descriptors_loop_length_offs-2);
		//print_HEX(buffer_tmp, 100);

		//////check this section is full or not , or has finished all events
		if(offs+offs_tmp <= MAX_EIT_SECTION_LEN-50)
		{
			memcpy(p->buffer+offs,buffer_tmp,offs_tmp);
			offs+=offs_tmp;
			p_data_loop->num_ps++;
			if(p_data_loop->num_all==p_data_loop->num_ps)
			{
				break;
			}
				
		}
		else//this section is full
		{
			break;
		}
	//	printf("all:%d ,ps:%d\n",p_data_loop->num_all,p_data_loop->num_ps);
		
		
	}


	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	//printf("			---section长度:%d\n",offs+4-3);
	//printf("			---打包完成!\n");
	//print_HEX(p->buffer+1,2);
	

	

	offs+=4;//for CRC
	p->len=offs;

	//print_HEX(p->buffer, offs);
	
	return 0;

	
}


static int CreateOneTable_EITPF(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id,int base)
{
	int ret;
	unsigned short section_number=0;
	int num=0;
	int i;
	unsigned short service_stream_id=0;
	//version_number
	//unsigned char version_number=0;
	//version_number=get_version_number_task(table_id, baseid, streamID);

	printf("@@@section打包开始......\n");
	//get service info
	char data_service[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	if(table_id==actual_EIT_PF_table_id)
	{
		sprintf(sql,"SELECT id,service_id from xepg_sdt_extension where transport_stream_id=%s",streamID);
	}
	else if(table_id==other_EIT_PF_table_id)
	{
		sprintf(sql,"SELECT id,service_id from xepg_sdt_extension where transport_stream_id!=%s and transport_stream_id!=%d",streamID,special_stream_id);
	}
	num=mysql_get_data(&mysql, sql,data_service);
	printf("@@@共有table_extension(service)%d个\n",num);
	
	for(i=0;i<num;i++)
	{
		//get streamid of this service
		printf("@@@service:%s\n",data_service[i][1]);
		sprintf(sql,"select transport_stream_id from xepg_nit_extension t where t.id in (select transport_stream_id from xepg_sdt_extension where id=%s)",data_service[i][0]);
		mysql_get_data(&mysql, sql,data);
		
		service_stream_id=atoi(data[0][0]);
		
		//curruct event
		CreateEIS_PF(table_id,0,&p_sbi[section_number+base],service_stream_id,data_service[i][0],(unsigned short)(atoi(data_service[i][1])),task_info.version_number);
		section_number++;

		//following event
		CreateEIS_PF(table_id,1,&p_sbi[section_number+base],service_stream_id,data_service[i][0],(unsigned short)(atoi(data_service[i][1])),task_info.version_number);
		section_number++;
		/*
		sprintf(logbuf,"ver[%d]ts[%s]tb[0x%x]sv[%s]sec[2]",version_number,stream_id,table_id,data_service[i][1]);
		proclog(logfd, mdname, logbuf);
		*/
		//PrintToFile(logfd,mdname, "ver[%d]ts[%s]tb[0x%x]sv[%s]sec[2]",version_number,stream_id,table_id,data_service[i][1]);
		
	}

	/* update last section number of every section */
	for(i=base;i<base+section_number;i++)
	{
		//*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}

	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	return section_number;
}


static int CreateTables_EITSC(unsigned char table_id,SEC_BUFFER_INFO *p_sbi,char *streamID,char *stream_id,int base)
{
	int ret;
	unsigned char last_section_number=0;
	unsigned int all_section_number=0;
	unsigned char table_section_number=0;
	unsigned char service_section_number=0;

	int start_hour=0,end_hour=0,sc_end_hour=0;
	
	int num=0,n=0;
	int i,j,k,l,m;
	char flag=0;
	unsigned short service_stream_id=0;
	char service_streamID[32];

	
	printf("@@@section打包开始......\n");
	//version_number
	//unsigned char version_number=0;
	//version_number=get_version_number_task(table_id, baseid, streamID);
	
	//get service info
	char data_service[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	if(table_id==actual_EIT_SC_table_id)
	{
		sprintf(sql,"SELECT id,service_id from xepg_sdt_extension where transport_stream_id=%s",streamID);
	}
	else if(table_id==other_EIT_SC_table_id)
	{
		sprintf(sql,"SELECT id,service_id from xepg_sdt_extension where transport_stream_id!=%s and transport_stream_id!=%d",streamID,special_stream_id);
	}
	num=mysql_get_data(&mysql, sql,data_service);
	printf("@@@共有table_extension(service)%d个\n",num);
	DATA_LOOP data_loop;
	all_section_number=0;

	//printf("---循环顺序  service->table->segment->section\n");
	for(i=0;i<num;i++)//for every service
	{
		flag=0;

		printf("@@@service:%s\n",data_service[i][1]);
		
		//get streamid of this service
		
		sprintf(sql,"select id,transport_stream_id from xepg_nit_extension t where t.id in (select transport_stream_id from xepg_sdt_extension where id=%s)",data_service[i][0]);
		mysql_get_data(&mysql, sql,data);
		service_stream_id=atoi(data[0][1]);
		strcpy(service_streamID,data[0][0]);

		//get end hour
		sprintf(sql,"select eit_send_hours from xepg_nit_extension where id=%s",service_streamID);
		n=mysql_get_data(&mysql, sql,data);
		if(n)
			sc_end_hour=atoi(data[0][0]);
		
		service_section_number=0;
		for(j=table_id;j<=table_id+0xF;j++)//for every table
		{
			table_section_number=0;
			printf("@@@@table_id:0x%x\n",j);
			for(k=0;k<32;k++)//for every segment  (3 hours per circle)
			{
			//	printf("		---segment:%d\n",k);
				start_hour=(j-table_id)*32*3+k*3;
				end_hour=start_hour+3;
				sprintf(sql,"select id,start_time-interval 8 hour,duration,running_status,free_CA_mode, event_name,event_id from xepg_eit_extension where service_id=%s and addtime(start_time,duration) between NOW()+interval %d hour and NOW()+interval %d hour and start_time < NOW() + interval %d hour order by start_time",data_service[i][0],start_hour,end_hour,sc_end_hour);
				data_loop.num_all=mysql_get_data(&mysql, sql, data_loop.data);
				
				if(!data_loop.num_all && !flag)
				{
					sprintf(sql,"select count(*) from xepg_eit_extension where service_id=%s and start_time > NOW() + interval %d hour and start_time < NOW() + interval %d hour ",data_service[i][0],end_hour,sc_end_hour);
					mysql_get_data(&mysql, sql, data);
					if(!atoi(data[0][0]))
					{
						//printf("finished segment:%d\n",k);
						flag=1;
						//l=0;//for count last_section_number
						//break;
					}	
				}
				
				data_loop.num_ps=0;
				for(l=0;l<8;l++)//for every section
				{
					//printf("---------section_numnber:%d\n",k*8+l);
					CreateEIS_SC(j,k*8+l,&p_sbi[all_section_number+base],(unsigned short)(atoi(data_service[i][1])),service_stream_id,task_info.version_number,&data_loop);
					table_section_number++;
					service_section_number++;
					all_section_number++;
					//printf("table_section_number:%d\n",table_section_number);
					//printf("all_section_number:%d\n",all_section_number);

					if(data_loop.num_all==data_loop.num_ps)//for making empy section
						break;
				}
				
				//update segment_last_section_number for every section
				for(m=0;m<l+1;m++)
				{
					*(unsigned char *)(p_sbi[all_section_number+base-l-1+m].buffer+12)=8*k+l;
					//printf("p_sbi[%d]=section_number:%d\n",all_section_number+base-l-1+m,k*8+m);
				}
				last_section_number=k*8+l;
			}
			
			//update last_section_number for every section
			//printf("k:%d,l:%d,last_section_number:%d\n",k,l,k*8+l);
			for(m=0;m<table_section_number;m++)
			{
				*(unsigned char *)(p_sbi[all_section_number+base-table_section_number+m].buffer+7)=last_section_number;
			}
			
			if(flag)
				break;
			
		}
		
		//update last_table_id
		/*
		for(m=0;m<all_section_number;m++)
		{
			*(unsigned char *)(p_sbi[base+m].buffer+13)=j;
		}
		*/

		if(j > table_id+0xF)
			j=table_id+0xF;

		for(m=0;m<service_section_number;m++)
		{
		//	printf("set last_table_id: service:%s,last_table_id:0x%x\n",data_service[i][1],j);
			*(unsigned char *)(p_sbi[all_section_number+base-service_section_number+m].buffer+13)=j;
		}
		
		/*
		sprintf(logbuf,"ver[%d]ts[%s]tb[0x%x~0x%x]sv[%s]hour[%d]",version_number,stream_id,table_id,j,data_service[i][1],sc_end_hour);
		proclog(logfd, mdname, logbuf);
		*/
		//PrintToFile(logfd, mdname, "ver[%d]ts[%s]tb[0x%x~0x%x]sv[%s]hour[%d]",version_number,stream_id,table_id,j,data_service[i][1],sc_end_hour);
	}

	/* update crc of every section */
	for(i=base;i<base+all_section_number;i++)
	{
		//*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}
	//printf("end calcing CRC\n");

	printf("@@@所有section打包完成!共%d个section\n\n",all_section_number);
	return all_section_number;
}



static DTBOneTable_EIT()//create actual EIT or other EIT
{

	//====update packing flag====
	update_packing_flag(1,task_info.task_pid,task_info.task_table_id);
	
	//====Get task info====
	if(get_task_info(&task_info)<0)
		exit(0);

	
	//====Create SECTIONS====
	SEC_BUFFER_INFO sbi[256*3*150];
	memset(sbi,0,256*3*150*sizeof(SEC_BUFFER_INFO));
	
	int section_number=0;
	if( (task_info.table_id==actual_EIT_PF_table_id) || (task_info.table_id==other_EIT_PF_table_id) )
	{
		section_number=CreateOneTable_EITPF(task_info.table_id,sbi,task_info.extendID,task_info.extend_id,0);
	}
	else if( (task_info.table_id==actual_EIT_SC_table_id) || (task_info.table_id==other_EIT_SC_table_id) )
	{
		section_number=CreateTables_EITSC(task_info.table_id,sbi,task_info.extendID,task_info.extend_id,0);
	}
	else
	{
		printf("@@@错误!table_id:%d 不属于EIT!\n",task_info.table_id);
		exit(0);
	}


	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);
	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);

}

main(int argc,char **argv)
{
	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_EIT();
	
}


#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_TOT_SECTION_LEN 1024
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="TOT";
static char sql[512];

static char foreigntype=6;

unsigned int printlevel = 0;
TASK_INFO task_info;
static char CreateTOS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p)
{
	printf("@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_TOT_SECTION_LEN);
	memset(p->buffer,0,MAX_TOT_SECTION_LEN);
	

	unsigned int offs=0;
	int len=0;
	unsigned int descriptors_loop_length_offs=0;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;
	offs++;

	/* section_syntax_indicator 	reserved_future_use reserved*/
	*(unsigned char *)(p->buffer+offs)=0x70;
	offs++;

	//make room for section length
	offs++;

	//MJD 16
	//get cur time
	char ts[32];
	time_t tt;
	char tmStr[24];
	
	tt=time(0)-8*3600;
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	printf("@@@@@格林威治时间:%s\n",ts);
	
	*(unsigned short*)(p->buffer+offs)=htons(CalcMJDByDate(ts));
	printf("MJD: ");
	print_HEX(p->buffer+offs,2);
	offs+=2;

	//time BCD 24
	strcpy(tmStr,ts+11);
	trimTimeStr(tmStr);
	HexString_to_Memory(tmStr,6, p->buffer+offs);
	offs+=3;
	printf("@@@@@BCD: ");
	print_HEX(p->buffer+offs-5,5);


	//reserved_future_use
	*(unsigned char*)(p->buffer+offs)=0xF0;
	
	//record offs of descriptors_loop_length
	descriptors_loop_length_offs=offs;

	//make room for descriptors_loop_length
	offs+=2;

	//descriptors
	printf("@@@@@正在打包第一层描述符......\n");
	sprintf(sql,"Select attributename,descriptorid,bitnum,attributevalue,hex,descriptornum from xepg_descriptor_relation_view t Where t.foreigntype='%x' and t.foreignid=0  and t.layer='1' Order By descriptorid,groupid,seq",foreigntype);
	len=build_descriptors_memory(sql,p->buffer+offs);
	offs+=len;

	//update descriptors_loop_length_offs
	set_bits16(p->buffer+descriptors_loop_length_offs, offs-descriptors_loop_length_offs-2);

	/*section length*/
	set_bits16(p->buffer+1, offs+4-3);
	printf("@@@@@section长度:%d\n",offs+4-3);
	printf("@@@@@打包完成!\n");

	

	offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}

static int CreateOneTable_TOT(SEC_BUFFER_INFO *p_sbi)
{
	int num;
	int i;
	unsigned char section_number=0;

	/*
	//version_number
	unsigned char version_number=0;
	sprintf(sql,"SELECT version_num from xepg_task where status=0 and table_id IN (select id from xepg_tableid where table_id='%x') and pid in (select id from xepg_pid where baseid='%d' and extend='%s')",table_id,baseid,stream_id);
	num=mysql_get_data(&mysql, sql,data);
	if(num)
		version_number=(unsigned char)atoi(data[0][0]);
	*/

	CreateTOS(task_info.table_id,section_number,&p_sbi[section_number]);
	section_number++;

	/* update last section number of every section */
	
	for(i=0;i<section_number;i++)
	{
		//*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}
	/*
	sprintf(logbuf,"ts[%s]tb[0x%x]sec[1]",stream_id,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	//PrintToFile(logfd,mdname, "ts[%s]tb[0x%x]sec[1]",stream_id,table_id);
	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	return section_number;
}

static DTBOneTable_TOT()
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
	section_number=CreateOneTable_TOT(sbi);
	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);

	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);
	
}

main(int argc,char **argv)
{

	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_TOT();
	
}


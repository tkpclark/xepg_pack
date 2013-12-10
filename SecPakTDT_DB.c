#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"


#define MAX_TDT_SECTION_LEN 1024
MYSQL mysql;
static char logbuf[2048];
int logfd=0;
char mdname[]="TDT";
static char sql[512];

TASK_INFO task_info;


//static char foreigntype=5;

unsigned int printlevel = 0;

static char CreateTDS(unsigned char table_id,unsigned char section_number,SEC_BUFFER_INFO *p)
{
	printf("@@@@section_number:%d正在打包......\n",section_number);
	p->buffer=malloc(MAX_TDT_SECTION_LEN);
	memset(p->buffer,0,MAX_TDT_SECTION_LEN);
	

	unsigned int offs=0;

	//tableid
	*(unsigned char *)(p->buffer)=table_id;
	offs++;

	/* section_syntax_indicator 	reserved_future_use reserved*/
	*(unsigned char *)(p->buffer+offs)=0x70;
	offs++;

	//make room for section length
	offs++;

	printf("@@@@@正在获取UTC时间......\n");
	//get cur time
	char ts[32];
	time_t tt;
	char tmStr[24];
	
	tt=time(0)-8*3600;
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	printf("@@@@@格林威治时间:%s\n",ts);
	
	//MJD 16
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

	/*section length*/
	set_bits16(p->buffer+1, offs-3);//没有CRC
	printf("@@@@@section长度:%d\n",offs-3);
	printf("@@@@@打包完成!\n");

	

	//offs+=4;//for CRC
	p->len=offs;
	return 0;

	
}

static int CreateOneTable_TDT(SEC_BUFFER_INFO *p_sbi)
{
	int num;
	int i;
	unsigned char section_number=0;
	printf("@@@section打包开始......\n");
	/*
	//version_number
	unsigned char version_number=0;
	sprintf(sql,"SELECT version_num from xepg_task where status=0 and table_id IN (select id from xepg_tableid where table_id='%x') and pid in (select id from xepg_pid where baseid='%d' and extend='%s')",table_id,baseid,stream_id);
	num=mysql_get_data(&mysql, sql,data);
	if(num)
		version_number=(unsigned char)atoi(data[0][0]);
	*/

	CreateTDS(task_info.table_id,section_number,&p_sbi[section_number]);
	section_number++;
	/*
	sprintf(logbuf,"ts[%s]tb[0x%x]sec[1]",stream_id,table_id);
	proclog(logfd, mdname, logbuf);
	*/
	//PrintToFile(logfd,mdname, "ts[%s]tb[0x%x]sec[1]",stream_id,table_id);
	/* update last section number of every section */
	/*
	for(i=0;i<section_number;i++)
	{
		*(unsigned char *)(p_sbi[i].buffer+7)=section_number-1;
		get_CRC(p_sbi[i].buffer,p_sbi[i].len-4);
		//print_HEX(sbi[i].buffer,sbi[i].len);

		//write_section(sbi,section_number);
	}

	*/

	printf("@@@所有section打包完成!共%d个section\n\n",section_number);
	return section_number;
}


static DTBOneTable_TDT()
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
	section_number=CreateOneTable_TDT(sbi);
	
	//====PACK====
	//pack_and_dtb(section_number, sbi, task_info.baseid,task_info.table_id,task_info.ds_server,task_info.ds_port,task_info.pid,task_info.cltid,task_info.interval,task_info.extend_id);
	pack2tsfile(section_number, sbi, task_info.pid,task_info.cltid);
	
	
}

main(int argc,char **argv)
{

	read_argv_1(argc, argv);
	pack_init();
	DTBOneTable_TDT();
	
}


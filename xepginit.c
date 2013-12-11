#include "xepg.h"

extern MYSQL mysql;
extern int logfd;
extern TASK_INFO task_info;

void xepg_procquit(void)
{
	//====update packing flag====
	update_packing_flag(0,task_info.task_pid,task_info.task_table_id);
	printf("@@@���ڹر����ݿ�����......\n");
	mysql_close(&mysql);
	printf("@@@�������н������˳�.\n");
}

void pack_init()
{
	char ip[16];
	char user[10];
	char psw[10];
	char db_name[20];

	
	read_config("db_ip",ip);
	read_config("db_user",user);
	read_config("db_psw",psw);
	read_config("db_name",db_name);

	//open log
	char logfile[128];
	read_config("logfile",logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("@@@����־�ļ�%sʧ��!%s\n",logfile,strerror(errno));
		exit(0);
	}
	printf("@@@�����������ݿ�......\n@@@%s %s %s\n",ip, user,db_name);
	mysql_create_connect(&mysql, ip, user,psw,db_name);

	//register quit function
	if(atexit(&xepg_procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}

	
}


void read_argv_1(int argc,char **argv)
{
	printf("@@@���ڶ�ȡ�����в���......\n");
	if(argc!=3)
	{
		printf("@@@��������!\n");
		printf("format:%s pid table_id\n",argv[0]);
		exit(0);
	}
	printf("@@@taskid: %s %s \n",argv[1],argv[2]);
	strcpy(task_info.task_pid,argv[1]);
	strcpy(task_info.task_table_id,argv[2]);
	//strcpy(task_info.ds_server,argv[3]);
	//task_info.ds_port=atoi(argv[4]);

	


	//printf("======packing cltid:%s %s======\n\n\n",task_pid,task_table_id);
	
}
/*
void read_argv_2(int argc,char **argv)
{
	if(argc < 3)
	{
		printf("arguments error!\n");
		exit(0);
	}	
	
	strcpy(ds_server,argv[1]);
	ds_port=atoi(argv[2]);
	if(argc > 3)
	{
		if(!strcmp(argv[3],"all"))
		{
			DTB_all_flag=1;
		}
		else
		{
			strcpy(DTB_stream_id,argv[3]);
		}
	}
}
*/


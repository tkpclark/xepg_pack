已经弃用，功能由updater包含

#include "mysqllib.h"
#include <sys/time.h>
#include <signal.h>
#include "xepg.h"
#include "mysqllib.h"

MYSQL mysql;
int logfd=0;
static char logbuf[2048];
char mdname[]="timerupdater";

static int slp_tm=1;
static unsigned int timecycle = 1;
	
unsigned int printlevel = 0;	

static void acalarm(int signo)
{
	timecycle++;
	return;
}

static init()
{
	char db_ip[32];
	read_config("db_ip",db_ip);
	//read_config("db_port",port);
	
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql,db_ip,"xepg","xepg","xepg",0,NULL,0))
	{
		sql_err_log(&mysql);
		exit(0);
	}

	#if 1
	struct sigaction signew;

	signew.sa_handler=acalarm;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGALRM,&signew,0);

		
	#endif

	//open log
	char logfile[]="../log/timerupdater.log";
	//read_config("logfile",logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("open logfile error!\n");
	}
	proclog(logfd,mdname,"starting...");
}


int main(int argc,char *argv[])
{
	char temp[1024];

	init();
	struct itimerval value;
	value.it_value.tv_sec=slp_tm;
	value.it_value.tv_usec=0;

	value.it_interval.tv_sec = slp_tm;    
    value.it_interval.tv_usec = 0; 
	
	setitimer(ITIMER_REAL, &value, NULL);
	char *log_base_dir="/home/app/pack/log";
	char packlogfile[128];
	int i,j;
	int num=0;
	DS_INFO ds_info;
	TASK_INFO task_info;
	char data_rec[600][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];

	//get ds info
	char *sql_ds="select ip,port from xepg_ds_config";
	ds_info.num=mysql_get_data(&mysql, sql_ds, ds_info.data_skt);

	
	//loop of tasks
	char *sql_task="SELECT pid,table_id,status,prog FROM xepg_task where ispacking=0";
	num=mysql_get_data(&mysql, sql_task, data_rec);
	
	while(1)
	{
		sprintf(temp,"select pid,table_id,prog from xepg_task where status=1 and ispacking=0 and scantime != 0 and %d MOD scantime=0",timecycle);
		num=mysql_get_data(&mysql,temp,data);
		for(i=0;i<num;i++)//tasks which needs to be updated
		{
			strcpy(task_info.task_pid,data[i][0]);
			strcpy(task_info.task_table_id,data[i][1]);
			if(get_task_info(&task_info)<0)
			{
				continue;
			}
			sprintf(packlogfile,"%s/%s_%s_%s.log",log_base_dir,data[i][2],task_info.task_pid,task_info.task_table_id);
			sprintf(temp,"./%s %s %s > %s",data[i][2],task_info.task_pid,task_info.task_table_id,packlogfile);
			proclog(logfd,mdname,temp);
			system(temp);
			for(j=0;j<ds_info.num;j++)//ds
			{
			
				DST_send_cmd1(task_info.cltid,task_info.interval,atoi(task_info.extend_id),task_info.baseid,ds_info.data_skt[i][0],atoi(ds_info.data_skt[i][1]));
			}

		}
		sleep(slp_tm);
	}
	
}




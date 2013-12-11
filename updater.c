
#include "xepg.h"


MYSQL mysql;
int logfd=0;
static char logbuf[2048];
char mdname[]="updater";

static int slp_tm=1;
static unsigned long counter=0;
static unsigned second_counter=0;


unsigned int printlevel = 0;	
static void acalarm(int signo)
{
	return;
}

static init()
{
	char ip[16];
	char user[10];
	char psw[10];
	char db_name[20];

	
	read_config("db_ip",ip);
	read_config("db_user",user);
	read_config("db_psw",psw);
	read_config("db_name",db_name);

	
	mysql_create_connect(&mysql, ip, user,psw,db_name);

	#if 1
	struct sigaction signew;

	signew.sa_handler=acalarm;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGALRM,&signew,0);

		
	#endif

	/*
	//open log
	char logfile[64];
	read_config("logfile",logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("open logfile error!\n");
	}
	//read_config("server_ip", ip);
	//read_config("server_port", port);
	
	proclog(logfd,mdname,"starting...\n");
	*/
}

static sync_task_status(TASK_INFO *p_task_info,int status,char *prog,DS_INFO *p_ds_info,int scantime)
{

	int i;
	
	char cmd[128];
	char temp[100] = {0};

	int	cmdresult = 0;
	
	char packlogfile[128];
	char *log_base_dir="/home/app/pack/log";

	//open log
	//sprintf(mdname,"%d",getpid());
	char _logfile[64];
	//int logfd;
	sprintf(_logfile,"../log/task_%s_%s.log",p_task_info->task_pid,p_task_info->task_table_id);
	logfd=open(_logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("child %d open logfile %s failed!",getpid());
		exit(0);
	}
	dup2(logfd,0);


	trunc_big_file(logfd,1000000);
	/*
	PrintToFile(logfd,mdname,"任务同步开始...");
	PrintToFile(logfd,mdname,"任务信息:cltid[%d]taskid[%s_%s]baseid[%d]version[%d]tablename[%s]base_type[%d]pid[%d]tableid[%d]packfor[%d](1:ts 2:service)extendID[%s]extend_id[%s]interval[%d]networkID[%d]network_id[%d]",
		p_task_info->cltid,
		p_task_info->task_pid,
		p_task_info->task_table_id,
		p_task_info->baseid,
		p_task_info->version_number,
		p_task_info->tablename,
		p_task_info->base_type,
		p_task_info->pid,
		p_task_info->table_id,
		p_task_info->packfor,
		p_task_info->extendID,
		p_task_info->extend_id,
		p_task_info->interval,
		p_task_info->networkID,
		p_task_info->network_id
	//	p_task_info->ds_server,
	//	p_task_info->ds_port
	);
	*/

	/*
	if(status==0)
	{
		PrintToFile(logfd,mdname,"数据库状态:停止");
	}
	else if(status==-1)
	{
		PrintToFile(logfd,mdname,"数据库状态:更新");
	}
	else if(status==1)
	{
		PrintToFile(logfd,mdname,"数据库状态:运行");
	}
	*/

	
	//PrintToFile(logfd,mdname,"打包程序:%s,second_counter:%d,scantime:%d",prog,second_counter,scantime);

	if((status==-1) || (scantime && second_counter%scantime==0) )
	{
		//更新时重新打包或定时任务到达更新时刻也重新打包
		PrintToFile(logfd,mdname,"正在打包...");
		//get log file path
		sprintf(packlogfile,"%s/%s_%s_%s.log",log_base_dir,prog,p_task_info->task_pid,p_task_info->task_table_id);
		/*
		//update logfile field in xepg_task
		sprintf(temp,"update xepg_task set logfile='%s' where pid=%s and table_id=%s",packlogfile,data_rec[i][0],data_rec[i][1]);
		proclog(logfd,mdname,temp);
		mysql_query(&mysql, temp);
		*/


		//revolve pack programme
		sprintf(temp,"./%s %s %s > %s",prog,p_task_info->task_pid,p_task_info->task_table_id,packlogfile);
		proclog(logfd,mdname,temp);
		system(temp);
	}

	//get ds
	if(p_ds_info->num==0)
	{
		//PrintToFile(logfd,mdname,"没有配置DS,同步结束!");
		return;
	}
//	PrintToFile(logfd,mdname,"开始向DS发布......");
	for(i=0;i<p_ds_info->num;i++)//every ds
	{
	//	PrintToFile(logfd,mdname,"DS[%d]:%s|%d",i+1,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1]));

		if((status==-1) || (scantime && second_counter%scantime==0) )
		{
			//PrintToFile(logfd,mdname,"update task,status:%d\n",status);
			DST_send_cmd1(p_task_info->cltid,p_task_info->interval,atoi(p_task_info->extend_id),p_task_info->baseid,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1]));
			continue;
		}
		
		else if(status==0) 
		{
			cmdresult =DST_send_cmd2(p_task_info->cltid,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1])); //query
			if(cmdresult<-1)
			{
				continue;
			}
			if(cmdresult==1)
			{
				//PrintToFile(logfd,mdname,"stop task,status:%d\n",status);
				DST_send_cmd10(p_task_info->cltid,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1]));//stop
				/*
				sprintf(logbuf,"stop: %s %s %s %s %s",data[i][6],data[i][2],data[i][3],ip,port);
				proclog(logfd,mdname,logbuf);
				*/
			}
			else
			{
				;//PrintToFile(logfd,mdname,"状态正常!");
			}
		}
			
		else if(status==1) 
		{
			
			cmdresult =DST_send_cmd2(p_task_info->cltid,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1])); //query
			if(cmdresult<-1)
			{
				//PrintToFile(logfd,mdname,"[%d]cmdresult:%d\n",p_task_info->cltid,cmdresult);
				continue;
			}
			if(cmdresult!=1)
			{
				//PrintToFile(logfd,mdname,"sync task,status:%d\n",status);
				//sprintf(logbuf,"cmdresult:%d,cltid:%d\n",cmdresult,atoi(cltid));
				//proclog(logfd,mdname,logbuf);
				DST_send_cmd1(p_task_info->cltid,p_task_info->interval,atoi(p_task_info->extend_id),p_task_info->baseid,p_ds_info->data_skt[i][0],atoi(p_ds_info->data_skt[i][1]));
			}
			else
			{
				;//PrintToFile(logfd,mdname,"状态正常!");
			}
		}

	}
	//PrintToFile(logfd,mdname,"同步结束。\n");
	//alarm(0);
	//pthread_exit(NULL);

}
static routin()
{
	int i,j;
	int n;
	int task_num=0;
	int pid=0;
	DS_INFO ds_info;
	TASK_INFO task_info;
	char data_rec[600][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	printf("second_counter:%d\n",++second_counter);


	

	struct itimerval value;
	value.it_value.tv_sec=slp_tm;
	value.it_value.tv_usec=0;
	
	setitimer(ITIMER_REAL, &value, NULL);

	//get ds info
	char *sql_ds="select ip,port from xepg_ds_config";
	ds_info.num=mysql_get_data(&mysql, sql_ds, ds_info.data_skt);

	
	//loop of tasks
	char *sql_task="SELECT pid,table_id,status,prog,scantime FROM xepg_task where ispacking=0";
	task_num=mysql_get_data(&mysql, sql_task, data_rec);


	for(i=0;i<task_num;i++)
	{
		
	
		///////////
		for(j=0;j<50;j++)
		{
			n=waitpid(-1,NULL,WNOHANG);
			if(n==0)
				break;
			if(n<0)
			{
				printf("wait pid return %d,%s\n",n,strerror(errno));
				break;
			}
		}
		//////////
		printf("\n\n=====task:%s %s,status:%d,prog:%s,scantime:%d=====\n",data_rec[i][0],data_rec[i][1],atoi(data_rec[i][2]),data_rec[i][3],atoi(data_rec[i][4]));

		strcpy(task_info.task_pid,data_rec[i][0]);
		strcpy(task_info.task_table_id,data_rec[i][1]);
		if(get_task_info(&task_info)<0)
		{
			continue;
		}
		//
		pid=fork();
		if(pid==0)//child
		{
			sync_task_status(&task_info,atoi(data_rec[i][2]),data_rec[i][3],&ds_info,atoi(data_rec[i][4]));
			exit(0);
		}
		else if(pid>0)
		{
			//printf("forked child %d\n",pid);
		}
		else
		{
			printf("fork failed!\n");
			exit(0);
		}
		
			

		//sync_task_status(data_rec[i][0],data_rec[i][1], atoi(data_rec[i][2]),data_rec[i][3]);
		/*
		
		
		*/

		
		//sleep(1);
	}
	//wait pid
	sleep(slp_tm);
	

}
main()
{

	//sent by transport stream
	/*
	char *sql1="select	t1.pid,t1.table_id,t4.transport_stream_id,CONV(t3.table_id,16,10),t1.status,t1.scantime,t1.prog\
				from xepg_task t1, xepg_pid t2,xepg_tableid t3, xepg_nit_extension t4\
				where (t1.prog like '%EIT%' or t1.prog like '%NIT%' or t1.prog like '%SDT%' or t1.prog like '%BAT%'  or t1.prog like '%TDT%' or t1.prog like '%TOT%' or t1.prog like '%MNIT%') \
				and 	t1.ispacking=0 and t1.pid=t2.id and t1.table_id=t3.id and t2.extend=t4.id ";

	//sent by service
	char *sql2="select t1.pid,t1.table_id,t4.service_id,CONV(t3.table_id,16,10),t1.status,t1.scantime,t1.prog \
				from xepg_task t1, xepg_pid t2,xepg_tableid t3, xepg_sdt_extension t4 \
				where  (t1.prog like '%FST%' or t1.prog like '%PFT%' or t1.prog like '%PAT%') \
				and  t1.ispacking=0 and t1.pid=t2.id and t1.table_id=t3.id and t2.extend=t4.id";
	*/


	//chdir(dir);

	
	init();

	while(1)
	{	
		
		routin();
	}
}

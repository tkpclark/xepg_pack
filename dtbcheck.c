#include "xepg.h"
#include "mysqllib.h"
#include <sys/time.h>

#define MAX_SERVICE	500


MYSQL mysql;
int logfd=0;
char mdname[]="dtbcheck";

unsigned int printlevel = 0;

char rec[MAX_SERVICE][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LONG];

//service's attribute,include:service id,schedule id,and imgurl
typedef struct
{
	unsigned int service_id;
	int id;
	char imgurl[128];
} SERVICE_SC;



typedef struct 
{
	int service_num;
	SERVICE_SC	service[MAX_SERVICE];
}ServiceSC_S;

ServiceSC_S cur_service={0},last_service={0},ad_service_sc={0},ad_sc_7s={0};
ServiceSC_S cur_service_pst ={0},last_service_pst={0},pst_service_sc={0},pst_sc_7s={0};
ServiceSC_S cur_service_pf={0},last_service_pf={0},pf_service={0};//last_service and cur_service :all sdt's service,cur_service_sc:all current 
																// schedule's service,service_sc_7s:all 7's after schedule's service																
int	packflag;

//get all sdt service
void	GetAllService(unsigned int *sc_num,SERVICE_SC *sc)
{
	char sql[512];
	int i;
	
	sprintf(sql,"select id from xepg_sdt_extension where service_id IN (select distinct service_id  from xepg_sdt_extension where \
						transport_stream_id!=65535)");
	*sc_num=mysql_get_data_long(&mysql,sql,rec);
	for(i =0;i<*sc_num;i++)
	{
		sc[i].service_id = atoi(rec[i][0]);
	}
}

//get current schedule's service id and schedule id
void GetCurSC(char * table,unsigned int *sc_num,SERVICE_SC *sc)
{
	char sql[512];
	int i;
	
	sprintf(sql,"SELECT service_id,id  FROM %s where NOW() between start_time and end_time",table);
	*sc_num = mysql_get_data_long(&mysql,sql,rec);
	for(i=0;i<*sc_num;i++)
	{
		sc[i].service_id = atoi(rec[i][0]);
		sc[i].id = atoi(rec[i][1]);
	}	
}



//get current schedule's service id and schedule id
void GetCurPF(char * table,unsigned int *sc_num,SERVICE_SC *sc)
{
	char sql[512];
	int i;
	
	sprintf(sql,"SELECT service_id,id  FROM %s where NOW() >=  start_time and NOW() < start_time + interval HOUR(duration ) HOUR + interval MINUTE(duration ) MINUTE + interval SECOND(duration ) SECOND ",table);
	*sc_num = mysql_get_data_long(&mysql,sql,rec);
	for(i=0;i<*sc_num;i++)
	{
		sc[i].service_id = atoi(rec[i][0]);
		sc[i].id = atoi(rec[i][1]);
	}	
}


//get 7s after all schedule's service id and imgurl
void Get7sAfterSC(char * table,unsigned int *sc_num,SERVICE_SC *sc)
{
	char sql[512];
	int i;	

	sprintf(sql,"SELECT service_id ,imgurl FROM %s where NOW()+ INTERVAL 7 SECOND between start_time and end_time ",table);
	*sc_num = mysql_get_data_long(&mysql,sql,rec);
	
	for(i=0;i<*sc_num;i++)
	{
		sc[i].service_id = atoi(rec[i][0]);
		strcpy(sc[i].imgurl,rec[i][0]);
	}
}


//check if the service_id in the schedule 
int FindServiceSC(int service_id,ServiceSC_S *service)
{
	int i;
	
	for(i=0;i<(service->service_num);i++)
	{
		if(service_id==service->service[i].service_id)
			return i;
	}
	return -1;
} 


//check this service need update or not 
int IsServiceNeedPack(int serviceID,ServiceSC_S *sc,ServiceSC_S *sc_cur)
{
	int result = 0;
	
	result = FindServiceSC(sc->service[serviceID].service_id,sc_cur);
	
	if(result<0)
	{
		if(sc->service[serviceID].id>0)
		{
			sc->service[serviceID].id = -1;
			return 1;
		}
		else
			sc->service[serviceID].id = -1;
	}

	else//if programme start find imgurl=null,how do pack?
	{
		if(sc->service[serviceID].id!=sc_cur->service[result].id)
		{
			sc->service[serviceID].id = sc_cur->service[result].id;
			return 1;
		}
	}

	return 0;
}


//is 7s after is null schedule,if that,need update 
int IsServiceNeedPack_Add7s(int serviceID,ServiceSC_S *sc,ServiceSC_S *sc_7s)
{
	int result = 0;	

	result = FindServiceSC(sc->service[serviceID].service_id,sc_7s);

	if(result < 0)
	{
		if(sc->service[serviceID].id>0)
		{
			sc->service[serviceID].id = -1;
			return 1;
		}	
			
	}

	else
	{
		if(sc_7s->service[result].imgurl==NULL)
		{
			sc->service[serviceID].id = -1;
			return 1;
		}	
	}

	return 0;
	
}



//check all service need to update or not 
int	IfUpdate(ServiceSC_S *sc,ServiceSC_S *sc_cur,ServiceSC_S *sc_cur_7s,unsigned char *condition,char *tb_type)
{
	int i;
	int ifpack = 0;
	int packflag = 0;
	char temp[32] = {0};
	char *p = NULL;
	
	for(i = 0; i < sc->service_num;i++)//所有的service
	{
		
		if(sc_cur_7s==NULL)
			goto TT;
		ifpack = IsServiceNeedPack_Add7s(i,sc,sc_cur_7s);
		if(!ifpack)
TT:
		ifpack = IsServiceNeedPack(i,sc,sc_cur);

		if(ifpack)
		{
			memset(temp,0,sizeof(temp));
			sprintf(temp,"%d,",sc->service[i].service_id);
			strcat(condition,temp);
			PrintToFile(logfd, mdname, "[%s]service:[%d]排期发生变化 ",tb_type,sc->service[i].service_id);
		}

		packflag = packflag|ifpack;
	}

	if(packflag)
	{
		p = strrchr(condition,',');
		*p = '\0';
		//printf("conditon:%s\n",condition);
		//PrintToFile(logfd, mdname, "[%s]下列service排期发生变化: [%s]",tb_type,condition);
	}
	return packflag;
}


void ProcessService(ServiceSC_S *last_service,ServiceSC_S *cur_service)
{
	int i,j;
	int flag = 0;
	for(i = 0;i<MAX_SERVICE;i++)
	{
		for(j = 0;j<MAX_SERVICE;j++)
		{
			if(cur_service->service[i].service_id==last_service->service[j].service_id)
			{
				cur_service->service[i].id = last_service->service[j].id;
				strcpy(cur_service->service[i].imgurl,last_service->service[j].imgurl);
				flag = 1;
				break;
			}
		}

		if(flag)
		{
			flag = 0;
		}
		else
		{
			cur_service->service[i].id = 0;
			memset(cur_service->service[i].imgurl,0,128);
		}
	}
}



static init()
{
	char ip[16];
	char user[10];
	char psw[10];
	char db_name[20];

	//open log
	char logfile[]="../log/dtbcheck.log";
	//read_config("logfile",logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("open logfile error!file:%s\n",logfile);
		exit(0);
	}
	proclog(logfd,mdname,"starting...");
	read_config("db_ip",ip);
	read_config("db_user",user);
	read_config("db_psw",psw);
	read_config("db_name",db_name);
	
	
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql,ip,user,psw,db_name,0,NULL,0))
	{
		sql_err_log(&mysql);
		exit(0);
	}

}

main()
{
	int i;
	int num=0;
	char sql[4096];
	char condition[40960];
	int ifpack = 0;
	char temp[4096] = {0};
	unsigned int counter = 0;

				
	init();
	
	while(1)
	{
		/*******get NOW()*******/
		
		char curtime[32];
		mysql_get_data(&mysql,"select NOW()",data);
		strcpy(curtime,data[0][0]);
		/********epgad*********/
		{

			printf("=====epgad=====\n");
			if(!(counter%100))	//update memory service info  every 100 s
			{
				//PrintToFile(logfd, mdname, "get new service list!\n");
				memcpy(&last_service,&cur_service,sizeof(ServiceSC_S));
				memset(&cur_service,0,sizeof(ServiceSC_S));
				GetAllService(&(cur_service.service_num),cur_service.service);
				ProcessService(&last_service,&cur_service);
			}
		
			memset(condition,0,sizeof(condition));
			
			GetCurSC("xepg_ad",&(ad_service_sc.service_num),ad_service_sc.service);
			Get7sAfterSC("xepg_ad",&(ad_sc_7s.service_num),ad_sc_7s.service);
			
			packflag = IfUpdate(&cur_service,&ad_service_sc,&ad_sc_7s,condition,"FST");

			
			if(packflag)
			{						
				
				sprintf(sql,"update xepg_task set status =-1 where status!=0 and table_id in (select id from xepg_tableid where table_id='af') and  pid IN (select id from xepg_pid where extend IN(%s))",condition);
				
				mysql_exec(&mysql,sql);
				/*
				if(!mysql_affected_rows(&mysql))
				{
					proclog(logfd,mdname,"[FST]没有配置相关任务执行更新!");
				}
				*/

			}

		}




		/********epgposter*********/
		{
			printf("=====epgposter=====\n");
			if(!(counter%100))	
			{
				//PrintToFile(logfd, mdname, "get new service list!\n");
				memcpy(&last_service_pst,&cur_service_pst,sizeof(ServiceSC_S));
				memset(&cur_service_pst,0,sizeof(ServiceSC_S));
				GetAllService(&(cur_service_pst.service_num),cur_service_pst.service);
				ProcessService(&last_service_pst,&cur_service_pst);
			}
			//condition that should be distributed now in the table xepg_ad
			//sprintf(condition,"dtbflag=0 and '%s' >= start_time and  '%s' < end_time ",curtime,curtime);
			memset(condition,0,sizeof(condition));
			GetCurSC("xepg_poster",&(pst_service_sc.service_num),pst_service_sc.service);
			Get7sAfterSC("xepg_poster",&(pst_sc_7s.service_num),pst_sc_7s.service);
			
			packflag = IfUpdate(&cur_service_pst,&pst_service_sc,&pst_sc_7s,condition,"PFT");

			
			if(packflag)
			{
				//FST
				sprintf(sql,"update xepg_task set status =-1 where status!=0 and table_id in (select id from xepg_tableid where table_id='ad') and  pid IN (select id from xepg_pid where extend IN(%s))",condition);
				
				mysql_exec(&mysql,sql);
				/*
				if(!mysql_affected_rows(&mysql))
				{
					proclog(logfd,mdname,"[PFT]没有配置相关任务执行更新!");
				}
				*/

			}
		}







		/********EIT PF*********/
		{
			printf("=====EITPF=====\n");
			if(!(counter%100))	
			{
				//PrintToFile(logfd, mdname, "get new service list!\n");
				memcpy(&last_service_pf,&cur_service_pf,sizeof(ServiceSC_S));
				memset(&cur_service_pf,0,sizeof(ServiceSC_S));
				GetAllService(&(cur_service_pf.service_num),cur_service_pf.service);
				ProcessService(&last_service_pf,&cur_service_pf);
			}
			memset(condition,0,sizeof(condition));
			 //sprintf(condition,"dtbflag=0 and  '%s' >=  start_time and '%s' < start_time + interval HOUR(duration ) HOUR + interval MINUTE(duration ) MINUTE + interval SECOND(duration ) SECOND ",curtime,curtime);
			//update status in version
			GetCurPF("xepg_eit_extension",&(pf_service.service_num),pf_service.service);
			packflag = IfUpdate(&cur_service_pf,&pf_service ,NULL ,condition,"EIT");
			if(packflag)
			{			
				sprintf(sql,"select pid from xepg_task where status!=0 and table_id in (select id from xepg_tableid where table_id='4e') and  pid IN\
							(\
								select id from xepg_pid where extend IN\
								(\
									select  transport_stream_id from xepg_sdt_extension where id in\
									(\
										%s\
									)\
								)\
							)",condition);
				
				num=mysql_get_data(&mysql,sql,data);
				if(num)
				{

					//update status
					for(i=0;i<num;i++)
					{
						sprintf(sql,"update xepg_task set status=-1 where (pid='%s' and table_id in (select id from xepg_tableid where table_id='4e') and status!=-1 and status!=0) or (pid!='%s' and table_id in (select id from xepg_tableid where table_id='4f') and status!=-1 and status!=0) ",data[i][0],data[i][0]);
						mysql_exec(&mysql,sql);

						/*
						if(!mysql_affected_rows(&mysql))
						{
							proclog(logfd,mdname,"[EIT]没有配置相关任务执行更新!");
						}
						*/
					}
				}
			}

		}


		sleep(1);
		counter++;
	}
}


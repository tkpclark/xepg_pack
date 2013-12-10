
#include "mysqllib.h"

extern char mdname[];
extern int logfd;
static char logbuf[1024];

void sql_err_log(MYSQL *mysql)
{
	//proclog(logfd,mdname,mysql_error(mysql));
	PrintToFile(logfd,mdname,mysql_error(mysql));
	mysql_close(mysql);
}
void mysql_create_connect(MYSQL *mysql,char *ip,char *usr,char *pwd,char *dbname)
{
	mysql_init(mysql);
	if(!mysql_real_connect(mysql,ip,usr,pwd,dbname,0,NULL,0))
	{
		sql_err_log(mysql);
		exit(0);
	}
}
void mysql_exec(MYSQL *mysql,char *buf)
{
	unsigned int myerr;
	int i;
	i=1;
	printf("%s\n",buf);
	MYRE:
	if (mysql_query(mysql,buf))
	{
		myerr=mysql_errno(mysql);
		if(myerr==CR_SERVER_GONE_ERROR || myerr==CR_SERVER_LOST)
		{	/*
			sprintf(logbuf,"redo SQL %d time",i);
			proclog(logfd,mdname,logbuf);
			*/
			PrintToFile(logfd,mdname,"redo SQL %d time",i);
			i++;
			if(i>3)
				exit(0);
			goto MYRE;
		}
		else
		{	/*
			sprintf(logbuf,"error sql:%s",buf);
			proclog(logfd,mdname,logbuf);
			*/
			PrintToFile(logfd,mdname,"error sql:%s",buf);
			sql_err_log(mysql);
			//exit(0);
		}
	}
	else
	{
		return ;
	}
}
int mysql_get_data(MYSQL *mysql,char *sql,char p[][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN])
{
	int row_num=0,field_num=0;
	char snull[]="NULL";
	MYSQL_ROW row;
	MYSQL_RES *result;
	
	mysql_exec(mysql,"set names gb2312");

	//printf("%s\n",sql);
	mysql_exec(mysql,sql);

	result=mysql_store_result(mysql);
	row_num=mysql_num_rows(result);
	field_num=mysql_num_fields(result);
	printf("row_num:%d,field_num:%d\n",row_num,field_num);
	if(!row_num)
	{
		mysql_free_result(result);
		return 0;
	}


	int i=0,j=0;
	while(row=mysql_fetch_row(result))
	{
		printf("[%d]  ",i+1);
		for(j=0;j<field_num;j++)
		{
			if(row[j]!=NULL)
			{
				if(strlen(row[j]) >= DATA_MAX_REC_LEN)
				{
					printf("TOO LONG");
				}
				else
				{
			 		strcpy(p[i][j],row[j]);
					printf("%-20s",p[i][j]);
				}
			}
			else
			{
				p[i][j][0]=0;
				printf("%-20s",snull);
			}
		}
		i++;
		printf("\n");
	}
	mysql_free_result(result);
	printf("\n");

	return row_num;
}



int mysql_get_data_long(MYSQL *mysql,char *sql,char p[][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LONG])
{
	int row_num=0,field_num=0;
	char snull[]="NULL";
	MYSQL_ROW row;
	MYSQL_RES *result;
	
	mysql_exec(mysql,"set names gb2312");

	//printf("%s\n",sql);
	mysql_exec(mysql,sql);

	result=mysql_store_result(mysql);
	row_num=mysql_num_rows(result);
	field_num=mysql_num_fields(result);
	printf("row_num:%d,field_num:%d\n",row_num,field_num);
	if(!row_num)
	{
		mysql_free_result(result);
		return 0;
	}


	int i=0,j=0;
	while(row=mysql_fetch_row(result))
	{
		printf("[%d]  ",i+1);
		for(j=0;j<field_num;j++)
		{
			if(row[j]!=NULL)
			{
				if(strlen(row[j]) >= DATA_MAX_REC_LONG)
				{
					printf("TOO LONG");
				}
				else
				{
			 		strcpy(p[i][j],row[j]);
					printf("%-20s",p[i][j]);
				}
			}
			else
			{
				p[i][j][0]=0;
				printf("%-20s",snull);
			}
		}
		i++;
		printf("\n");
	}
	mysql_free_result(result);
	printf("\n");

	return row_num;
}


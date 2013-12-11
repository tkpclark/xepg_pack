#define DATA_MAX_REC_NUM 200
#define DATA_MAX_FIELD_NUM 12
#define DATA_MAX_REC_LEN 64
#define DATA_MAX_REC_LONG 128



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <errno.h>
#include <errmsg.h>



void sql_err_log(MYSQL * mysql);

void mysql_exec(MYSQL *mysql,char *buf);

int mysql_get_data(MYSQL *mysql,char *sql,char p[][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN]);

static char data[DATA_MAX_REC_NUM][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];


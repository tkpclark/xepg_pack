#define PKG_SIZE 188
#define MAX_SECTION_NUM 256
#define MAX_EXTENSION_NUM 100
#define MAX_EXTENSION_LEN 100
#define MAX_ATTRI_LEN 1024

#define SPECIAL_STREAM_ID 65535

#define CLR_BIT(x,y) ((x)&=~( 0x0001<<(y)))
#define SET_BIT(x,y) ((x)|=(0x0001<<(y)))
#define CPL_BIT(x,y) ((x)^= (0x0001<<(y)))  //get ~
#define LET_BIT(x,y,z) ((x)=(x)&(~(1<<(y)))|((z)<<(y)))// write z on the yth on x

#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "color.h"
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include "mysqllib.h"

typedef enum 
{
   RESERVE,
   	
   LVL_DBG,/*detail program info*/

    LVL_INFO, /*program info*/

    LVL_ERROR,/*common error*/
		
    LVL_FATAL,/*fatal error*/
    
    LVL_NONE
 }debug_level_e;


//#include "ocilib.h"

typedef struct
{
    char *p[MAX_SECTION_NUM];
    unsigned int num;

}SECTION_INFO;

typedef struct
{

	char sync_byte[8];
	char transport_error_indicator[1];
	char payload_unit_start_indicator[1]; 
	char transport_priority[1];
	char pid[13]; 
	char transport_scrambling_control[2];
	char adaptation_field_control[2]; 
	char continuity_counter[4]; 

} TS_HEADER;


typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char b0[1];
	char reserved0[2];
	char section_length[12];
	char trasport_stream_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];
} PAT_HEADER;

typedef struct
{
	char program_number[16];
	char reserved[3];
	char pmt_pid[13];
}PAT_PROGRAM;


typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char network_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];
	//char reserved_future_use1[4];
	//char network_descriptors_length[12];
}NIT_HEADER;

typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char service_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];
	//char reserved_future_use1[4];
	//char network_descriptors_length[12];
}EIT_HEADER;

typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char bouquet_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];
	//char reserved_future_use1[4];
	//char network_descriptors_length[12];
}BAT_HEADER;


typedef struct
{
	char descriptor_tag[8];
	char descriptor_length[8];
	char frequency[32];
	char reserved_future_use[12];
	char FEC_outer[4];
	char modulation[8];
	char symbol_rate[28];
	char FEC_inner[4];
}CABLE_DELIVERY_SYSTEM_DESCRIPTOR;

typedef struct
{
	char descriptor_tag[8];
	char descriptor_length[8];
}SERVICE_LIST_DESCRIPTOR;

typedef struct
{
	char service_id[16];
	char service_type[8];
}SERVICE_LIST_DESCRIPTOR_INFO;

typedef struct
{
      char table_id[8];
      char section_syntax_indicator[1];
      char reserved_future_use0[1];
      char reserved0[2];
      char section_length[12];
      char transport_stream_id[16];
      char reserved1[2];
      char version_number[5];
      char current_next_indiator[1];
      char section_number[8];
      char last_section_number[8];
      //char original_network_id[16];
      //char reserved_future_use1[8];
}SDT_HEADER;

typedef struct
{
	char service_id[16];
	char reserved_future_use[6];
	char EIT_schedule_flag[1];
	char EIT_present_following_flag[1];
	char running_status[3];
	char free_CA_mode[1];
	char descriptors_loop_length[12];
}SDT_SERVICE;

typedef struct
{
	unsigned char *p;
	short pid;
	unsigned int pkg_num;
	//unsigned int wrote_pkg_num;
	unsigned char count_base;
	unsigned char counter;
}DATA_FILE_TS_LIST;

typedef struct
{
	unsigned short id;
	char value[MAX_EXTENSION_LEN];
	unsigned short pid;
}EXTENSION;

typedef struct
{
	unsigned short id;
	unsigned short tsid;
}STREAM;
typedef struct
{
	char HEX;
	char bin[5];
}HEX_BIN;

typedef struct
{
	char HEX;
	char ints ;
}HEX_INT;

typedef struct
{
	unsigned int descriptorid;
	unsigned int descriptornum;
	unsigned int bitnum;
	unsigned int HEX;
	char attributename[32];
	char attributevalue[MAX_ATTRI_LEN];
	
	//char pDesBinString[MAX_ATTRI_LEN*8];
}DESCRIPTOR_INFO;

typedef struct
{
	unsigned int descriptorid;
	unsigned int descriptornum;
}DESCRIPTOR_KEY;

typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char reserved1[16];
	char reserved2[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];

}AAT_HEADER;

typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char trasport_stream_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];

}PDT_HEADER;

typedef struct
{
	char table_id[8];
	char section_syntax_indicator[1];
	char reserved_future_use0[1];
	char reserved0[2];
	char section_length[12];
	char file_sub_id[16];
	char reserved1[2];
	char version_number[5];
	char current_next_indicator[1];
	char section_number[8];
	char last_section_number[8];

}FTT_HEADER;


/*
typedef struct
{
	unsigned int filesize;
	unsigned int block_offset;
	unsigned short block_size;
	
}FTS_FILE_INFO;
*/
typedef struct
{
	int pic_id;
	int sec_id;
	char *buffer;
	unsigned int len;
}SEC_BUFFER_INFO;

typedef struct
{
	char data[6000][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	unsigned int num_all;
	unsigned int num_ps;
	//unsigned char segment_last_section_number;
	//char finish_flag;
}DATA_LOOP;

typedef struct
{
	char task_pid[16];
	char task_table_id[16];
	int cltid;
	int base_type;
	int interval;
	unsigned char version_number;
	int packfor;// 1:ts 2:service
	unsigned short pid;
	unsigned char table_id;
	unsigned int baseid;
	char extendID[32];
	char extend_id[32];//ts or servece
	//char ds_server[32];
	//int ds_port;
	char tablename[32];
	unsigned short network_id;
	int networkID;
}TASK_INFO;

typedef struct
{
	int num;
	char data_skt[10][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
}DS_INFO;



unsigned char BinString8_to_Int1(char *p);//8位二进制串变1位16进制整数
void BinString_to_Memory(char *BinString,char *Memory,int length);
char* HexChar_to_BinString4(char HexChar,char *BinString4);//a HEX charactor to 4 bin string
char HexChar_to_int1(char HexChar);//one hex charactor to a int
int HexString_to_Memory(char *HexString,int len,char *Memory);
char* HexString_to_BinString(char *HexString,int len,char *BinString);
char* DecString_to_BinString(char *DecString,int DecStringLen,char*BinString,int BinStringLen);
char* String_to_BinString(char *String,int len,char *BinString);
void reverse_short(short *value);
void set_bits16(char *p,short value);
void get_TS_header(char *pTShdr,short pid);
int section_pak_1(char *p,int len,short pid,char *sec_ts_buffer);
//void err_handler(OCI_Error *err);
void compose_asst(char *stradd,int strsize,char *info,char *pw);
int CRC32_Byte(int preg,int x);
int CRC32(char *buf,int size);
void print_HEX(char *p,int length);
unsigned char get_len_of_descriptor(DESCRIPTOR_INFO *p1,DESCRIPTOR_KEY *p2,int rec_num);
unsigned int get_diff_of_descriptors(DESCRIPTOR_INFO *p1,int rec_num,DESCRIPTOR_KEY *p2);
void convert_des_info(DESCRIPTOR_INFO *p1,int rec_num);
int get_descriptor_info(char *foreignid,char *buffer);
off_t get_file_size(int fd);
unsigned short get_pid(char *p);
char is_empty_buf(char *buf);
void remove_data_from_file(short pid);
void write_data_to_file(char *sec_ts_buffer,int length);
void DBG_Printf( debug_level_e level, char *format, ... );
void  PrintToFile(int fd,char *mdname,char *format, ...);

//static const char *TRANS_FILE="trans.ts";
//static const char *DATA_FILE="data.ts";







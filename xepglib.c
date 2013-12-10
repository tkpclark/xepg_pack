#define MAX_DES_NUM 200

#include "xepg.h"
#include "DST_send.h"
#include "mysqllib.h"

extern MYSQL mysql;
static char logbuf[2048];
extern int logfd;
extern char mdname[];
extern unsigned int	printlevel;

/*
void proclog(int fd, char *mdname,char *str)
{
	char ts[32];
	char buf[1024];
	time_t tt;
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%x %X",(const struct tm *)localtime(&tt));
	sprintf(buf,GREEN"[%s]"NONE YELLOW"[%s]"NONE"%s\n",ts,mdname,str);
	write(fd,buf,strlen(buf));
}
*/
void proclog(int fd, char *mdname,char *str)
{
	char ts[32];
	char buf[1024];
	time_t tt;
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%x %X",(const struct tm *)localtime(&tt));
	sprintf(buf,"[%s][%s]%s\n",ts,mdname,str);
	write(fd,buf,strlen(buf));
}
void strrep(char *str,const char *src,const char *des)
{
	char *p=NULL;
	char tmp[4096];
	while(1)
	{
		p=strstr(str,src);
		if(!p)
			break;
		strcpy(tmp,p+strlen(src));
		strcpy(p,des);
		strcat(str,tmp);
	}
}
void print_HEX(char *p,int length)
{
	int i;
	for(i=0;i<length;i++)
	{
		printf("%02X ",*(unsigned char*)(p+i));
	}
	printf("\n");
}

off_t get_file_size(int fd)
{
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		return statbuf.st_size;
	}
	else
	{
		printf("ALERT:get file stat error!\n");
		return 0;
	}

}

off_t get_file_size_f(char *path)
{
	int fd;
	off_t size=0;
	fd=open(path,0);
	if(fd<0)
	{
		printf("open %s failed !\n",path);
		exit(0);
	}
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		size= statbuf.st_size;
	}
	else
	{
		printf("ALERT:get file stat error!\n");
		size= 0;
	}
	close(fd);
	return size;
}


void reverse_short(short *value)
{
	char ch=0;
	ch=*(unsigned char*)(value);
	*((unsigned char*)(value))=*((unsigned char*)(value)+1);
	*((unsigned char*)(value)+1)=ch;
}
void set_bits16(char *p,short value)
{
	short _value=value;
	//printf("%d[0x%02x]\n ",value,value);
	//printf("[0x%04X]:",*(unsigned short*)(p));
	reverse_short(&_value);
	*(short*)(p)=*(short*)(p)|_value;
	//printf("[0x%04X]\n",*(unsigned short*)(p));
}

void get_TS_header(char *pTShdr,short pid)
{
	
	char tmp[64];
	
	TS_HEADER ts_header;
	memcpy(ts_header.sync_byte,"01000111",sizeof(ts_header.sync_byte)); /*!< Synchronization byte. */
	memcpy(ts_header.transport_error_indicator,"0",sizeof(ts_header.transport_error_indicator)); /*!< Transport stream error indicator, see \ref MPEG_TS_TRANSPORT_ERROR_INDICATOR for more details. */
	memcpy(ts_header.payload_unit_start_indicator,"0",sizeof(ts_header.payload_unit_start_indicator)); /*!< Payload unit start indicator, see \ref MPEG_TS_PAYLOAD_UNIT_START_INDICATOR for more details. */
	memcpy(ts_header.transport_priority,"0",sizeof(ts_header.transport_priority)); /*!< Transport stream priority, see \ref MPEG_TS_TRANSPORT_PRIORITY for more details. */
	memcpy(ts_header.pid,"0000000000000",sizeof(ts_header.pid)); /*!< Program ID, bits 12:8. */
	memcpy(ts_header.transport_scrambling_control,"00",sizeof(ts_header.transport_scrambling_control)); /*!< Transport stream scrambling control, see \ref MPEG_TS_SCRAMBLING_CTRL for more details. */
	memcpy(ts_header.adaptation_field_control,"01",sizeof(ts_header.adaptation_field_control)); /*!< Transport stream Adaptation field control, see \ref MPEG_TS_ADAPTATION_FIELD for more details. */
	memcpy(ts_header.continuity_counter,"0000",sizeof(ts_header.continuity_counter));
	
	compose_asst((char*)&ts_header,sizeof(ts_header),"TS HEADER",pTShdr);
	set_bits16(pTShdr+1,pid);
}
unsigned short get_pid(char *p)
{
	unsigned short pid=0;
	char tmp[2];
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,p+1,2);
	tmp[0]=tmp[0]&0x1F;
	reverse_short((unsigned short *)tmp);
	pid=*(unsigned short *)tmp;
	return pid;

}
unsigned short CalcMJD(unsigned short Y,unsigned short M,unsigned short D)
{
        short L=0;
        if( M==1 || M==2 )
                L=1;
        else
                L=0;

        return 14956+D+(int)((Y-L)*365.25)+(int)((M+1+L*12)*30.6001);
}
unsigned short CalcMJDByDate(char *dtime)
{
        //data must be like the format 2000-00-00 00:00:00
        if(!strlen(dtime))
			return 0;
        char dt[64];
        strcpy(dt,dtime);
        printf("dt:%s\n",dt);
        char date[32];
        unsigned short Y,M,D;

        strcpy(date,(char*)strtok(dt," "));
        printf("date:%s\n",date);

        char *p;
        char tmp[32];
        strcpy(tmp,(char*)strtok(date,"-"));
        Y=atoi(tmp)-1900;
        strcpy(tmp,(char*)strtok(NULL,"-"));
        M=atoi(tmp);
        strcpy(tmp,(char*)strtok(NULL,"-"));
        D=atoi(tmp);


        printf("Y:%d\nM:%d\nD:%d\n",Y,M,D);

        return CalcMJD(Y,M,D);

}

char is_empty_buf(char *buf)
{
	int i;
	for(i=0;i<PKG_SIZE;i++)
	{
		if(*(buf+i))
			return 0;
	}
	return 1;
}
/*
void remove_data_from_file(short pid)
{
	int fd=0;
	short _pid=0;
	fd=open(DATA_FILE,2);
	if(fd<0)
	{
		printf("open %s error!\n",DATA_FILE);
		exit(0);
	}
	char buf[PKG_SIZE];
	char zero_buf[PKG_SIZE];
	memset(zero_buf,0,sizeof(zero_buf));

	int n=0;
	int i=0;
	//printf("remove pid[%04X] from %s\n",(unsigned short)pid,DATA_FILE);
	while(1)
	{
		//printf("[%d] ",i);
		memset(buf,0,sizeof(buf));
		n=read(fd,buf,sizeof(buf));
		//printf("[%d]",n);
		if(!n)
		{
			printf("end\n");
			break;
		}
		_pid=get_pid(buf);
		//printf("pid[%04X] [%04x] ",_pid,pid);
		if(_pid==pid)
		{
			lseek(fd,-PKG_SIZE,SEEK_CUR);
			write(fd,zero_buf,sizeof(zero_buf));
			//printf("removed\n");
			
		}
		else
		{
			//printf("\n");
		}
		i++;
	}
	close(fd);
	
}
void write_data_to_file_middle(char *whl_ts_buf,int length)
{
	int fd=0;
	char *p=whl_ts_buf;
	fd=open(DATA_FILE,2);
	if(fd<0)
	{
		printf("open %s error!\n",DATA_FILE);
		exit(0);
	}
	char buf[PKG_SIZE];

	printf("writing DATA_FILE...\n");
	int i=0;
	int n=0;
	while(1)
	{
		//printf("[%d] ",i);
		memset(buf,0,sizeof(buf));
		n=read(fd,buf,sizeof(buf));
		//printf("[%d]",n);
		if(!n)//arrive the end of the file
		{
			write(fd,p,length-(p-whl_ts_buf));
			//printf("wrote %d bytes at the end of the file\n",length-(p-whl_ts_buf));
			break;
		}
		if(is_empty_buf(buf))
		{
			lseek(fd,-PKG_SIZE,SEEK_CUR);
			write(fd,p,PKG_SIZE);
			p+=PKG_SIZE;
			//printf("wrote 188 bytes here,%d left\n",length-(p-whl_ts_buf));
			
			if(p==whl_ts_buf+length)
				break;
			
		}
		else
		{
			//printf("not empty here!\n");
		}
		i++;
	}
	close(fd);
}
*/
int section_pak_1(char *p,int len,short pid,char *sec_ts_buffer)
{
	//printf("begin to pak the section...\n");
	char buffer[len+188+1];
	memset(buffer,0,sizeof(buffer));
	memcpy(buffer+1,p,len);
	len=len+1;//length plus point field 

	int pkg_num=0;
	if(len%(PKG_SIZE-4))
		pkg_num=(len/(PKG_SIZE-4))+1;
	else
		pkg_num=len/(PKG_SIZE-4);
	//printf("TS package num:%d\n",pkg_num);


	/* stuff the buf */
	//printf("stuffing the package... \n");
	memset(buffer+len,0xFF,(PKG_SIZE-4)* (pkg_num)-len);
	//printf("started from [%d],stuffed [%d]bytes altogether\n",len,(PKG_SIZE-4)* (pkg_num)-len);
	
	
	/* TS header */
	char ts_hdr_buf[4];
	memset(ts_hdr_buf,0,sizeof(ts_hdr_buf));
	//printf("composing TS header...\n");
	get_TS_header(ts_hdr_buf,pid);

	int i;

	
	//the fisrt 188
	LET_BIT(ts_hdr_buf[1],6,1); //set the payload_unit_start_indicator to 0 for the TS header of the 2nd to the last package
	memcpy(sec_ts_buffer,ts_hdr_buf,4);
	memcpy(sec_ts_buffer+4,buffer,PKG_SIZE-4);
	//printf("[package0]\n");
	//print_HEX(sec_ts_buffer,PKG_SIZE);
	
	//the second to the end 188
	//ts_hdr_buf[1]-=0x40;//set the payload_unit_start_indicator to 0 for the TS header of the 2nd to the last package
	LET_BIT(ts_hdr_buf[1],6,0); //set the payload_unit_start_indicator to 0 for the TS header of the 2nd to the last package
	for(i=1;i<pkg_num;i++)
	{
		memcpy(sec_ts_buffer+i*PKG_SIZE,ts_hdr_buf,4);
		memcpy(sec_ts_buffer+i*PKG_SIZE+4,buffer+i*(PKG_SIZE-4),PKG_SIZE-4);

		//printf("[package%d]\n",i);
		//print_HEX(sec_ts_buffer+i*PKG_SIZE,PKG_SIZE);
	}

	
	return pkg_num;
}

Pak2TS(TS_BUFFER_INFO *p_tbi,SEC_BUFFER_INFO* p_sbi,unsigned int section_number,unsigned short pid)
{
	/* pak for every section */
	printf("@@@正在为所有的section打ts包(188)......\n");
	int i;
	p_tbi->len=0;
	int pkg_num=0;
	char sec_ts_buffer[PKG_SIZE*(4096/184+1)];
	for(i=0;i<section_number;i++)
	{
		//printf("section number:%d\n",i);
		memset(sec_ts_buffer,0,sizeof(sec_ts_buffer));
		pkg_num=section_pak_1(p_sbi[i].buffer,p_sbi[i].len, pid, sec_ts_buffer);
		free(p_sbi[i].buffer);
		//printf("pkgnum:%d\n",pkg_num);
		//p_tbi->buffer=realloc(p_tbi->buffer,p_tbi->len+PKG_SIZE*pkg_num);
		
		memcpy(p_tbi->buffer+p_tbi->len,sec_ts_buffer,PKG_SIZE*pkg_num);
		/*
		printf("tbi:\n");
		print_HEX(p_tbi->buffer, PKG_SIZE*pkg_num);
		printf("sec_ts_buffer:\n");
		print_HEX(sec_ts_buffer,PKG_SIZE*pkg_num);
		*/
		p_tbi->len+=PKG_SIZE*pkg_num;
		//printf("tbilen:%d\n",p_tbi->len);
		//write_data_to_file(sec_ts_buffer,PKG_SIZE*pkg_num);
	}
}

/*
void err_handler(OCI_Error *err)
{
    printf("ERROR:code:[ORA-%05i]",OCI_ErrorGetOCICode(err));
    printf("ERROR:msg:[%s]",OCI_ErrorGetString(err));
    printf("ERROR:sql:%s",OCI_GetSql(OCI_ErrorGetStatement(err)));

}
*/
trimTimeStr(char *tm)
{
	//printf("triming...\n");
        printf("orig:%s\n",tm);
        tm[2]=tm[3];
        tm[3]=tm[4];
        tm[4]=tm[6];
        tm[5]=tm[7];
        tm[6]=0;
	printf("new:%s\n",tm);
}


void compose_asst(char *stradd,int strsize,char *info,char *pw)
{
	char tmp[1024];
	char pp[strsize];
	int length=strsize/8;
	
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,stradd,strsize);
	//printf("%s:%s[%d]\n",info,tmp,strlen(tmp));
	BinString_to_Memory(tmp,pp,length);
	
	memcpy(pw,pp,sizeof(pp)/8);

}
int is_this_descriptor(DESCRIPTOR_INFO *p1,DESCRIPTOR_KEY *p2)
{
	if( (p1->descriptorid==p2->descriptorid) && (p1->descriptornum==p2->descriptornum) )
		return 1;
	else
		return 0;
}
unsigned char get_len_of_descriptor(DESCRIPTOR_INFO *p1,DESCRIPTOR_KEY *p2,int rec_num)
{
	int i;
	unsigned int len=0;
	for(i=0;i<rec_num;i++)
	{
		//printf("i:%d d1:%d d2:%d",i,p1[i].descriptorid,des);
		//if(p1[i].descriptorid==des)
		if(is_this_descriptor(&p1[i],p2))
		{
			//printf("len1:%d ",len);
			len+=p1[i].bitnum;
			//printf("len1:%d ",len);
		}
		//printf("\n");
	}
	return (unsigned char)(len/8);
}

unsigned int get_diff_of_descriptors(DESCRIPTOR_INFO *p1,int rec_num,DESCRIPTOR_KEY * p2)
{
	int num=0;//number of descriptors
	int i,j;
	int flag=0;

		
	for(i=0;i<rec_num;i++)
	{
		flag=0;
		for(j=0;j<=num;j++)
		{
			//printf("$$$$$i:%d j:%d:%d %d\n",i,j,p1[i].descriptorid,p2[j]);
			if( (p1[i].descriptorid==p2[j].descriptorid) && (p1[i].descriptornum==p2[j].descriptornum) )
			{	
				flag=1;//it already exists
			}
			
		}
		if(!flag)
		{
			p2[num].descriptorid=p1[i].descriptorid;
			p2[num].descriptornum=p1[i].descriptornum;
			num++;
			//printf("num:%d\n",num);
			
		}
		

	}
	return num;
	
}

char *get_des_memory_value(DESCRIPTOR_INFO *p1,DESCRIPTOR_KEY *p2,int rec_num,char *pDesValue)
{
	int i;
	int offs=0;
	int bitnum=0;
	char *pBinString=NULL;
	char BinStringTmp[1024];
	for(i=0;i<rec_num;i++)
	{
		//if(p1[i].descriptorid==des)
		if(is_this_descriptor(&p1[i],p2))
		{
			printf("rec%d :  Name:%s Value:%s  bitnum:%d   des:%d   Hex:%d\n",i,p1[i].attributename,p1[i].attributevalue,p1[i].bitnum,p1[i].descriptorid,p1[i].HEX);
			if(p1[i].bitnum%8==0)
			{
				if(p1[i].HEX==10)
				{
					pBinString=malloc(p1[i].bitnum+1);
					memset(pBinString,0,p1[i].bitnum+1);
					DecString_to_BinString(p1[i].attributevalue,strlen(p1[i].attributevalue), pBinString, p1[i].bitnum);
					BinString_to_Memory(pBinString, pDesValue+offs, p1[i].bitnum/8 );
					printf("memory: ");
					print_HEX(pDesValue+offs, p1[i].bitnum/8);
					offs+=p1[i].bitnum/8;
					free(pBinString);
				}
				else if(p1[i].HEX==16)
				{
					HexString_to_Memory(p1[i].attributevalue,strlen(p1[i].attributevalue),pDesValue+offs );
					printf("memory: ");
					print_HEX(pDesValue+offs, p1[i].bitnum/8);
					offs+=p1[i].bitnum/8;

				}
				else if(p1[i].HEX==0)
				{
					memcpy(pDesValue+offs,p1[i].attributevalue,p1[i].bitnum/8);
					print_HEX(pDesValue+offs, p1[i].bitnum/8);
					offs+=strlen(p1[i].attributevalue);
				}
				else if(p1[i].HEX==2)
				{
					BinString_to_Memory(p1[i].attributevalue, pDesValue+offs, p1[i].bitnum/8);
					offs+= p1[i].bitnum/8;
				}
				else if(p1[i].HEX==11)//float   
				{
					HexString_to_BinString_FixedLen(p1[i].attributevalue,strlen(p1[i].attributevalue), BinStringTmp,p1[i].bitnum);
					BinString_to_Memory(BinStringTmp, pDesValue+offs,  p1[i].bitnum/8);
					offs+= p1[i].bitnum/8;	
				}
				else if(p1[i].HEX==12)//UTC    
				{
					char tmStr[24];
					
					//MJD 16
					*(unsigned short*)(pDesValue+offs)=htons(CalcMJDByDate(p1[i].attributevalue));
					offs+=2;

					//time BCD 24
					strcpy(tmStr,p1[i].attributevalue+11);
					trimTimeStr(tmStr);
					HexString_to_Memory(tmStr,6, pDesValue+offs);
					offs+=3;
				}
				else
				{
					printf("error in get_Des_memory_value\n");
				}

			}
			else//bitnum%8!=0
			{
				printf("begin to compose !8\n");
				memset(BinStringTmp,0,sizeof(BinStringTmp));
				while(1)
				{
					if(p1[i].HEX==10)
					{
						DecString_to_BinString(p1[i].attributevalue, strlen(p1[i].attributevalue), BinStringTmp+bitnum, p1[i].bitnum);
					}
					else if(p1[i].HEX==16)
					{
						HexString_to_BinString(p1[i].attributevalue, strlen(p1[i].attributevalue),BinStringTmp+bitnum);
					}
					else if(p1[i].HEX==2)
					{
						memcpy(BinStringTmp+bitnum,p1[i].attributevalue, strlen(p1[i].attributevalue));
					}
					else if(p1[i].HEX==11)//float 
					{
						HexString_to_BinString_FixedLen(p1[i].attributevalue,strlen(p1[i].attributevalue), BinStringTmp+bitnum,p1[i].bitnum);
					}
					else
					{
						printf("a Hex type out of my thought,type:%d\n",p1[i].HEX);
					}
					
					bitnum+=p1[i].bitnum;
					printf("	rec:%d,bitnum:%d  all:%d  Binstring:%s\n",i,p1[i].bitnum,bitnum,BinStringTmp);
					if(bitnum%8==0)
						break;
					++i;
					if(i>=rec_num)
						return;
				}
				BinString_to_Memory(BinStringTmp, pDesValue+offs, bitnum/8);
				printf("	memory:");
				print_HEX(pDesValue+offs, bitnum/8);
				offs+=bitnum/8;
				bitnum=0;

			}

		}
	}
}
void des_info_count(DESCRIPTOR_INFO *p1,int rec_num)
{
	int i,j;
	char BingString;
	int len=0;
	char tmp[32];
	char length_tmp[64];
	printf("des_info_count\n");
	for(i=0;i<rec_num;i++)
	{
		/*
		if(p1[i].HEX==0)
		{
			String_to_BinString(p1[i].attributevalue,strlen(p1[i].attributevalue), p1[i].pDesBinString);
		}
		if(p1[i].HEX==16)
		{
			HexString_to_BinString(p1[i].attributevalue, strlen(p1[i].attributevalue), p1[i].pDesBinString);
		}
		if(p1[i].HEX==10)
		{
			DecString_to_BinString(p1[i].attributevalue, strlen(p1[i].attributevalue), p1[i].pDesBinString,p1[i].bitnum);
		}

		*/
		
		if(p1[i].bitnum==0)
		{
			if(p1[i].HEX==0)
				p1[i].bitnum=strlen(p1[i].attributevalue)*8;
			if(p1[i].HEX==16)
				p1[i].bitnum=strlen(p1[i].attributevalue)*4;
		}
		if(  (strlen(p1[i].attributename) >=5) && !strncmp(p1[i].attributename+strlen(p1[i].attributename)-5,"_name",5) )
		{
			//c --> 0x13
			//e -->-x10
			if(p1[i].attributevalue[0]=='c')
			{
				//printf("language : name[%s]value[%s]\n",p1[i].attributename,p1[i].attributevalue);
				p1[i].attributevalue[0]=0x13;
			}
			else if(p1[i].attributevalue[0]=='e')
			{
				//printf("language : name[%s]value[%s]\n",p1[i].attributename,p1[i].attributevalue);
				p1[i].attributevalue[0]=0x13;
				
			}

			
		}
		/*
		if(  (strlen(p1[i].attributename) >=7) && !strncmp(p1[i].attributename+strlen(p1[i].attributename)-7,"_length",7) )
		{
			memset(length_tmp,0,sizeof(length_tmp));
			strncpy(length_tmp,p1[i].attributename,strlen(p1[i].attributename)-7);
			printf("get length field:%d,%s find:%s\n",i,p1[i].attributename,length_tmp);
			for(j=0;j<rec_num;j++)
			{
				if( (p1[i].descriptorid==p1[j].descriptorid) && (j!=i) && !strcmp(p1[j].attributename,length_tmp))
				{
					sprintf(tmp,"%d",strlen(p1[j].attributevalue));
					printf("tmp:%s,%s,%s\n",tmp,p1[j].attributename,p1[j].attributevalue);
					strcpy(p1[i].attributevalue,tmp);
					break;
				}		
			}
		}
		*/

	}
}

int build_descriptors_memory(char *sql_des,char *buffer)
{
	//printf("build_descriptors_memory\n");

	unsigned int offs=0;
	int i;
	unsigned int rec_num=0;
	unsigned int dec_num=0;
	
	DESCRIPTOR_INFO des_info[MAX_DES_NUM*10];
	DESCRIPTOR_KEY des_key[MAX_DES_NUM];
	
	
	MYSQL_ROW row;
	MYSQL_RES *result;

	mysql_exec(&mysql,"set names gb2312");
	
	mysql_exec(&mysql,sql_des);

	result=mysql_store_result(&mysql);
	rec_num=mysql_num_rows(result);
	printf("descriptor records num:%d\n",rec_num);
	if(!rec_num)
	{
		printf("@@@@@@没有描述符信息!\n");
		mysql_free_result(result);
		return 0;
	}
	rec_num=0;
	while(row=mysql_fetch_row(result))
	{
		if(row[0]!=NULL)
			strcpy(des_info[rec_num].attributename,row[0]);
		else
			memset(des_info[rec_num].attributename,0,sizeof(des_info[rec_num].attributename));
		des_info[rec_num].descriptorid=atoi(row[1]);
		des_info[rec_num].bitnum=atoi(row[2]);
		des_info[rec_num].HEX=atoi(row[4]);
		des_info[rec_num].descriptornum=atoi(row[5]);
		if(row[3]!=NULL)
		{
			if(strlen(row[3]) >= sizeof(des_info[rec_num].attributevalue))
			{
				printf("descriptor is too long,ignore it and contine with the next \n");
				continue;
			}
			strcpy(des_info[rec_num].attributevalue,row[3]);
		}
		else
		{
			memset(des_info[rec_num].attributevalue,0,sizeof(des_info[rec_num].attributevalue));
		}
		//printf("%d: 0x%x %d %d %d %s %s\n",rec_num,des_info[rec_num].descriptorid,des_info[rec_num].descriptornum,des_info[rec_num].bitnum,des_info[rec_num].HEX,des_info[rec_num].attributename,des_info[rec_num].attributevalue);
		rec_num++;
	}
	mysql_free_result(result);

	memset(des_key,0,MAX_DES_NUM*(sizeof(DESCRIPTOR_KEY)));
	dec_num=get_diff_of_descriptors(des_info,rec_num,des_key);
	//printf("@@@@@@描述符个数:%d\n",dec_num);

	des_info_count(des_info,rec_num);

	printf("after des_info_count:\n");
	for(i=0;i<rec_num;i++)
	{
		printf("%d: 0x%x %d %d %d %s %s\n",i,des_info[i].descriptorid,des_info[rec_num].descriptornum,des_info[i].bitnum,des_info[i].HEX,des_info[i].attributename,des_info[i].attributevalue);
	}

	printf("\n\n\n");
	unsigned char des_len=0;
	char pDesValue[MAX_ATTRI_LEN];
	for(i=0;i<dec_num;i++)//every descriptor
	{
		
		//tag
		*(unsigned char *)(buffer+offs)=(unsigned char)des_key[i].descriptorid;
		printf("@@@@@@tag[%d][0x%X]",*(unsigned char*)(buffer+offs),*(unsigned char*)(buffer+offs));
		offs+=1;

		//length
		des_len=get_len_of_descriptor(des_info, &des_key[i], rec_num);
		*(unsigned char *)(buffer+offs)=des_len;
		printf("len[%d]\n",*(unsigned char*)(buffer+offs));
		offs+=1;

		//value
		memset(pDesValue,0,sizeof(pDesValue));
		get_des_memory_value(des_info, &des_key[i], rec_num,pDesValue);
		memcpy(buffer+offs,pDesValue,des_len);
		offs+=des_len;
		//printf("				@@@描述符值的二进制:",des_key[i].descriptorid);
		//print_HEX(pDesValue,des_len);
		printf("\n\n");

		
	}
	//printf("				@@@所有描述符二进制:");
	//print_HEX(buffer, offs);
	return offs;
	
}

void get_network_id(int *networkID,unsigned short *network_id)
{
	int num=0;
	///printf("@@@正在获取networkid......\n");
	char sql[256];
	sprintf(sql,"SELECT id,network_id FROM xepg_network where status=1 limit 1");
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("@@@获取网络ID失败!");
	}
	*networkID=atoi(data[0][0]);
	*network_id=(unsigned short)atoi(data[0][1]);
	//printf("@@@networkid主键:%d,值:%d\n",*networkID,*network_id);
	
}
/*
int get_cltid(unsigned char table_id,int baseid,int pid)
{
	int num=0;
	char sql[256];
	sprintf(sql,"SELECT pid,table_id from xepg_task where table_id IN (select id from xepg_tableid where table_id='%x') and pid in (select id from xepg_pid where baseid=%d and pid=%d)",table_id,baseid,pid);
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("no cltid!\n");
		return 0;
	}
	
	char tmp[32];
	sprintf(tmp,"%d%d",atoi(data[0][0]),atoi(data[0][1]));

	int cltid=0;
	cltid=atoi(tmp);

	printf("cltid:%d\n",cltid);

	return cltid;
}
*/
/*
int get_interval(unsigned char table_id,int baseid,int pid)
{
	int num=0;
	char sql[256];
	sprintf(sql,"SELECT cycletime from xepg_task where table_id IN (select id from xepg_tableid where table_id='%x') and pid in (select id from xepg_pid where baseid=%d and pid=%d)",table_id,baseid,pid);
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("no cltid and interval!\n");
		return 0;
	}

	unsigned int interval=0;
	interval=atoi(data[0][0]);

	printf("interval:%d\n",interval);

	return interval;
}
*/
/*
int get_pack_pid(int baseid,char *streamID,unsigned short *pid)
{
	int num=0;
	char sql[256];
	if(streamID==NULL)
		printf("streamID is NULL\n");
	else
		printf("streamID:%s\n",streamID);

	sprintf(sql,"select pid from xepg_pid where baseid=%d and extend='%s'",baseid,streamID);
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("no pid!\n");
		return -1;
	}

	*pid=atoi(data[0][0]);
	printf("pid:%d\n",*pid);
	return 0;
}
*/
unsigned char get_version_number_base(int baseid)
{
	int num=0;
	char sql[256];
	
	sprintf(sql,"SELECT version_num FROM `xepg_base` where id=%d",baseid);
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("no version_number!\n");
		return 0;
	}
		
	if(!num)
	{
		printf("no pid!\n");
		return 0;
	}
	unsigned char version_number=0;
	version_number=(unsigned char)atoi(data[0][0]);
	return version_number;
}
unsigned char get_version_number_task(char *task_pid,char *task_table_id)
{
	int num=0;
	char sql[256];

	sprintf(sql,"update  xepg_task set version_num=mod(version_num+1,32) where pid=%s and  table_id=%s",task_pid,task_table_id);
	mysql_exec(&mysql, sql);
	
	sprintf(sql,"SELECT version_num from xepg_task where pid=%s and  table_id=%s",task_pid,task_table_id);
	num=mysql_get_data(&mysql, sql,data);
	if(!num)
	{
		printf("no version_number!\n");
		return 0;
	}
		
	if(!num)
	{
		printf("no pid!\n");
		return 0;
	}
	unsigned char version_number=0;
	version_number=(unsigned char)atoi(data[0][0]);
	return version_number;
}

void update_packing_flag(char packflag,char *task_pid,char *task_table_id)
{
	char sql[512];
	printf("@@@正在更新打包标志位至%d......\n",packflag);
	sprintf(sql,"update xepg_task set ispacking=%d where pid=%s and  table_id=%s",packflag,task_pid,task_table_id);
	mysql_exec(&mysql,sql);

	if(packflag==1)
	{
		sprintf(sql,"update xepg_task set status=1 where status=-1 and pid=%s and table_id=%s",task_pid,task_table_id);
		mysql_exec(&mysql,sql);
	}
}
/*
int get_dtb_info(unsigned short *pid,int *cltid,int *interval,unsigned int baseid,char *streamID,char *stream_id,unsigned char table_id)
{

	//get pid	
	if(get_pack_pid(baseid, streamID,pid))
	{
		PrintToFile(logfd, mdname, "ts[%s]tb[%d] pid does not exist!",stream_id,table_id);
		printf("pid doesn't exist!\n");
		return -1;
	}


	//get cltid and  interval
	*cltid=get_cltid(table_id, baseid, *pid);
	if(!(*cltid))
	{
		PrintToFile(logfd, mdname, "ts[%s]tb[%d] cltid does not exist!",stream_id,table_id);
		printf("cltid doesn't exist!\n");
		return -1;
	}
	
	*interval=get_interval(table_id, baseid, *pid);
	if(!(*interval))
	{
		PrintToFile(logfd, mdname, "ts[%s]tb[%d] interval does not exist!",stream_id,table_id);
		printf("interval doesn't exist!\n");
		return -1;
	}

	return 0;
}
*/

typedef struct 
{
	unsigned short group_id;
	int	section_count;
	int	poision[256];
}Group_Info;



int	GetSbiInfo(SEC_BUFFER_INFO *psbi,int sbi_len,Group_Info *group,int *max_id,int *group_number)
{
	int i,j=0;
	unsigned char section_number_max=0;
	unsigned short group_id=0;
	unsigned short max_section_number_sub_id;

	printf("calc start!\n");
	
	for(i=0;i<sbi_len;i++)
	{
		//print_HEX((psbi+i)->buffer, 20);
		group_id  = ntohs(*(unsigned short *)(((psbi+i)->buffer)+3));
		if(group_id!=(group+j)->group_id)
		{
			if(i)
				j++;
			(group+j)->group_id = group_id;
		}
		if((group+j)->section_count>section_number_max)
		{
			section_number_max = (group+j)->section_count;
			*max_id = j;
		}
		(group+j)->section_count++;
	}

	*group_number = j+1;
	printf("calc end!\n");
}

void SbiBufferSort(SEC_BUFFER_INFO *psbi,int sbi_len)
{
	printf("@@@正在重新排序所有的seciton......\n");
	Group_Info	Group[1000] = {0};
	int max_id;
	int group_num;
	int i,j,k;
	int pos=0;
	int sbi_pos=0;
	int max_interval=0;
	int sbi_simu[sbi_len];
	SEC_BUFFER_INFO __sbi[sbi_len];

	for(i=0;i<sbi_len;i++)
	{
		sbi_simu[i]=-1;
	}

	GetSbiInfo(psbi,sbi_len,Group,&max_id,&group_num);
	printf("max_id:%d,group_num:%d,all_section:%d\n",max_id,group_num,sbi_len);

	/*
	for(i=0;i<group_num;i++)
	{
		printf("group[%d]:max_section:%d,group_id:0x%x\n",i,Group[i].section_count,Group[i].group_id);
	}
	*/

	if(Group[max_id].section_count==1)
	{
		printf("max group num == 1,don't need to sort\n");
		return;
	}
	//counter max (min(interval)) of the max group
	//formule
	max_interval=(sbi_len-Group[max_id].section_count)/(Group[max_id].section_count-1);
	printf("max_interval :%d\n",max_interval);
	if(!max_interval)
	{
		printf("max interval == 1,don't need to sort\n");
		return;
	}
	if(max_interval > sbi_len/2)
	{	max_interval/=2;
		printf("half max_interval :%d\n",max_interval);
	}

	if(max_interval > 300)
	{	max_interval=300;
		printf("max_interval :%d\n",max_interval);
	}
	else if(max_interval > 200)
	{	max_interval=200;
		printf("max_interval :%d\n",max_interval);
	}
	else if(max_interval > 100)
	{	max_interval=100;
		printf("max_interval :%d\n",max_interval);
	}
	


	//make a simulate sbi
	for(i=max_id;i<max_id+group_num;i++)
	{
		for(j=0;j<Group[i%group_num].section_count;j++)
		{
			for(k=0;k<sbi_len;k++)
			{
				if(sbi_simu[pos]!=-1)
				{
					pos=(pos+1)%sbi_len;
				}
				else
				{//this pos has not orded yet
					sbi_simu[pos]=i%group_num;
					pos=(pos+max_interval+1)%sbi_len;
					break;
				}
			}
			
			//Group[i].poision[j]=pos;

		}
	}
	/*
	for(i=0;i<sbi_len;i++)
	{
		printf("sbi_simu[%d]:%d\n",i,sbi_simu[i]);
	}
	*/


	//put sections in sbi into __sbi by order
	memset(__sbi,0,sbi_len*sizeof(SEC_BUFFER_INFO));
	sbi_pos=0;
	for(i=0;i<group_num;i++)
	{

		for(k=0;k<sbi_len;k++)
		{
			if(sbi_simu[k]==i)
			{
				memcpy(&__sbi[k],&psbi[sbi_pos],sizeof(SEC_BUFFER_INFO));
				//printf("%04d(__sbi) <- %04d(sbi)(group:%d)\n",k,sbi_pos,i);
				sbi_pos++;
			}
		}

	}

	memcpy(psbi,__sbi,sbi_len*sizeof(SEC_BUFFER_INFO));

	
}

void write_tbi_to_file(TS_BUFFER_INFO *ptbi,int cltid)
{
	int fd;
	char filename[64];

	int n;
	
	sprintf(filename,"../taskts/%d.ts",cltid);
	printf("@@@正在生成文件%s......\n",filename);
	fd=open(filename,O_CREAT|O_WRONLY|O_TRUNC,0600);
	if(fd==-1)
	{
		printf("@@@生成ts文件失败:%s\n",strerror(errno));
		exit(0);
	}

	n=write(fd,ptbi->buffer,ptbi->len);
	if(n!=ptbi->len)
	{
		printf("@@@写入ts文件失败,应写入%d,实际写入%d.%s\n",ptbi->len,n,strerror(errno));
		exit(0);
	}
	printf("@@@文件写入完毕,共%d字节\n",n);
	close(fd);
}
void pack2tsfile(int section_number,SEC_BUFFER_INFO* psbi,unsigned short pid,int cltid)
{
	int i;
	/************PACK************/
	TS_BUFFER_INFO tbi;
	tbi.buffer=malloc(section_number*4096*1.3);
	memset(tbi.buffer,0,section_number*4096*1.3);

	//print section bin
	printf("=======print section bin==========\n");
	for(i=0;i<section_number;i++)
	{
		printf("section_number:%d\n",i);
		print_HEX(psbi[i].buffer, psbi[i].len);
	}
	printf("=======print section bin over==========\n");

	
	if(section_number)
	{
		SbiBufferSort(psbi,section_number);
		Pak2TS(&tbi,psbi,section_number,pid);
		//you can write it to file if you want
		////write tbi to file
		//////
		printf("@@@ts包的总长度%d,个数:%d\n",tbi.len, tbi.len/PKG_SIZE);
	}
	else
	{
		/*
		set_bits16(tbi.buffer+1,pid);
		*(unsigned char *)(tbi.buffer+5)=table_id;
		tbi.len=6;
		*/
		printf("@@@ts包个数为0!\n");
		exit(0);
	}
	///write task ts file
	
	write_tbi_to_file(&tbi,cltid);


	
	//DST_send_cmd1(&tbi,cltid, interval,atoi(stream_id),baseid,ip, port);
	free(tbi.buffer);
}


void DBG_Printf( debug_level_e level, char *format, ... )
{  
	if(level<printlevel)
		return;

	    char buffer[4096];
	    va_list argPtr;
	    va_start(argPtr, format);
	    (void)vsprintf((char*)buffer, format, argPtr);
	    va_end(argPtr);
	    printf("%s \n ", (char*)buffer);
}


void  PrintToFile(int fd,char *mdname,char *format, ...)
{
	char buffer[1024*50];
	
	 va_list argPtr;
	 va_start(argPtr, format);
	 (void)vsprintf((char*)buffer, format, argPtr);
	 va_end(argPtr);
	proclog(fd, mdname,buffer);
}
int get_all_streams(unsigned short networkID,char data_stream[][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN])
{
	char sql[512];
	int stream_num=0;
	unsigned short special_stream_id=SPECIAL_STREAM_ID;
	sprintf(sql,"SELECT id, transport_stream_id from xepg_nit_extension where transport_stream_id!=%d and original_network_id=%d",special_stream_id,networkID);
	stream_num=mysql_get_data(&mysql, sql,data_stream);
	printf("stream_num:%d\n\n",stream_num);
	if(!stream_num)
	{
		printf("@@@该网络下没有流信息!\n");
		exit(0);
		
	}
	return stream_num;
}
int get_task_info(TASK_INFO *p_task_info)
{
	char sql[512];
	int num=0;
	char temp[32];
	char data_task_info[500][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];
	char data_extend_info[10][DATA_MAX_FIELD_NUM][DATA_MAX_REC_LEN];


	printf("@@@正在读取任务相关信息...\n");
	sprintf(temp,"%s%s",p_task_info->task_pid,p_task_info->task_table_id);
	p_task_info->cltid=atoi(temp);
	
	sprintf(sql,"select baseid,basetype,note,pid,table_id,extend, cycletime from xepg_task_info_view where task_pid='%s' and task_table_id='%s'",p_task_info->task_pid,p_task_info->task_table_id);
	num=mysql_get_data(&mysql, sql,data_task_info);
	if(num==0)
	{
		printf("@@@没有该任务的相关信息!\n");
		return -1;
	}
	if(num>=2)
	{
		printf("@@@警告!该任务在task表中存在多条记录,共%d条,本次至发送第一条!\n",num);
	}

	p_task_info->baseid=atoi(data_task_info[0][0]);
	p_task_info->base_type=atoi(data_task_info[0][1]);
	strcpy(p_task_info->tablename,data_task_info[0][2]);
	p_task_info->pid=atoi(data_task_info[0][3]);
	p_task_info->table_id=atoi(data_task_info[0][4]);
	strcpy(p_task_info->extendID,data_task_info[0][5]);
	p_task_info->interval=atoi(data_task_info[0][6]);


	//judge it's packing for stream or service ,then get stream_id or service_id 
	if(p_task_info->base_type<=6)
		p_task_info->packfor=1;
	else if(p_task_info->base_type==100)
		p_task_info->packfor=1;
	else if(p_task_info->base_type==54)
		p_task_info->packfor=2;
	else if(p_task_info->base_type==55)
		p_task_info->packfor=2;
	else if(p_task_info->base_type==7)
		;
	else if(p_task_info->base_type==8)
		;
	else 
	{
		printf("@@@base_type类型不在本程序的处理范围内，请配置该类型!\n");
		return -2;
	}


	//get extend_id
	if(p_task_info->packfor==1)
	{
		sprintf(sql,"select transport_stream_id from xepg_nit_extension where id=%s",p_task_info->extendID);
		num=mysql_get_data(&mysql, sql,data_task_info);
		if(!num)
		{
			printf("@@@主键为%s的流不存在!\n",p_task_info->extendID);
			return -3;
		}
		strcpy(p_task_info->extend_id,data_task_info[0][0]);
	}
	if(p_task_info->packfor==2)
	{
		sprintf(sql,"select service_id from xepg_sdt_extension where id=%s",p_task_info->extendID);
		num=mysql_get_data(&mysql, sql,data_task_info);
		if(!num)
		{
			printf("@@@主键为%s的service不存在!\n",p_task_info->extendID);
			return -4;
		}
		strcpy(p_task_info->extend_id,data_task_info[0][0]);
	}
	//judge it's version is by baseid or by task,then get version_number
	//update version by baseid:BAT MNIT NIT SDT
	//update version by taskid:EIT FST PFT
	//no version :TDT TOT
	if(p_task_info->base_type==4 ||p_task_info->base_type==1 ||p_task_info->base_type==100 ||p_task_info->base_type==2 )
	{
		p_task_info->version_number=get_version_number_base(p_task_info->baseid);
	}
	else if(p_task_info->base_type==3 ||p_task_info->base_type==54 ||p_task_info->base_type==55 )
	{
		p_task_info->version_number=get_version_number_task(p_task_info->task_pid,p_task_info->task_table_id);
	}
	else
	{
		p_task_info->version_number=0;
	}

	//get network_id
	get_network_id(&p_task_info->networkID,&p_task_info->network_id);

	

	//print task info
	printf("@@@cltid[%d]taskid[%s_%s]baseid[%d]version[%d]tablename[%s]base_type[%d]pid[%d]tableid[%d]packfor[%d](1:ts 2:service)extendID[%s]extend_id[%s]interval[%d]networkID[%d]network_id[%d]\n",
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
	
	return 0;
	
}

trunc_big_file(int fd,unsigned int size)
{
	if(get_file_size(fd) > size)
	{
		if(ftruncate(fd,0))
		{
			printf(logbuf,"truncate failed! %s",strerror(errno));
		}
	}
}

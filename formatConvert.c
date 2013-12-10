#include "xepg.h"
static HEX_INT hex_int[16]={
	{'0',0},
	{'1',1},
	{'2',2},
	{'3',3},
	{'4',4},
	{'5',5},
	{'6',6},
	{'7',7},
	{'8',8},
	{'9',9},
	{'A',10},
	{'B',11},
	{'C',12},
	{'D',13},
	{'E',14},
	{'F',15}

	};
static HEX_BIN hex_bin[16]={
	{'0',"0000"},
	{'1',"0001"},
	{'2',"0010"},
	{'3',"0011"},
	{'4',"0100"},
	{'5',"0101"},
	{'6',"0110"},
	{'7',"0111"},
	{'8',"1000"},
	{'9',"1001"},
	{'A',"1010"},
	{'B',"1011"},
	{'C',"1100"},
	{'D',"1101"},
	{'E',"1110"},
	{'F',"1111"}

	};
unsigned char BinString8_to_Int1(char *p)//8位二进制串变1位16进制整数
{
        int i;
        unsigned char Hex=0;
        for(i=0;i<8;i++)
        {
                if(p[7-i]=='1')
                Hex+=pow(2,i);
                //printf("Hex:%d\n",Hex);
        }
        return Hex;
}
void BinString_to_Memory(char *BinString,char *Memory,int length)
{

	char *p=BinString;
	char str[24];
	int i=0,j=0;
	for(i=0;i<length;i++)
	{
		memset(str,0,sizeof(str));
		strncpy(str,p,8);
		//printf("%s\n",str);
		p+=8;
		*(Memory+j++)=BinString8_to_Int1(str);
		//printf("======\n");
	}

}

char* HexChar_to_BinString4(char HexChar,char *BinString4)//a HEX charactor to 4 bin string
{
	
	int i;
	for(i=0;i<16;i++)
	{
		//printf("binxxx:%s\n",hex_bin[i].bin);
		if(toupper(HexChar)==hex_bin[i].HEX)
		{
			strcpy(BinString4,hex_bin[i].bin);
			//printf("%s",hex_bin[i].bin);
			break;
		}
	}
	return BinString4;
}
char HexChar_to_int1(char HexChar)//one hex charactor to a int
{
	
	int i;
	for(i=0;i<16;i++)
	{
		if(toupper(HexChar)==hex_int[i].HEX)
		{
			break;
		}
	}
	return hex_int[i].ints;

}
/*it is right
char HexChar_to_int1(char ch)
{
        if (ch>='0'&&ch<='9')
                return ch-48;
  else if (ch>='A'&&ch<='F')
                return ch-55;
  else if (ch>='a'&&ch<='f')
                return ch-87;
}
*/
int HexString_to_Memory(char *HexString,int len,char *Memory)
{
	int i=0;
	int j=0;
	for(i=0;i<len;i=i+2)
	{
		
		Memory[j++]=0x10*HexChar_to_int1(HexString[i])+HexChar_to_int1(HexString[i+1]);
	}
	Memory[j]=0;
	//printf("HexString_to_Memory:%d\n",j);
	return j;
	
}
char* HexString_to_BinString_FixedLen(char *HexString,int len,char *BinString,int BinStringLen)
{
	int i=0;
	char BinStringTmp[BinStringLen];
	memset(BinStringTmp,0,sizeof(BinStringTmp));
	BinString[0]=0;
	char BinString4[5];
	for(i=0;i<len;i++)
	{
		if(HexString[i]=='.')
			continue;
		strcat(BinStringTmp,HexChar_to_BinString4(HexString[i], BinString4));
	}
	memset(BinString,0x30,BinStringLen);
	strcpy(BinString+BinStringLen-strlen(BinStringTmp),BinStringTmp);
	printf("HexString_to_BinString_FixLen:     HexString:%s  BinString:%s \n",HexString,BinString);
	return BinString;
	
}

char* HexString_to_BinString(char *HexString,int len,char *BinString)
{
	int i=0;
	BinString[0]=0;
	char BinString4[5];
	for(i=0;i<len;i++)
	{
		strcat(BinString,HexChar_to_BinString4(HexString[i], BinString4));
	}
	printf("HexString_to_BinString:     HexString:%s  BinString:%s \n",HexString,BinString);
	return BinString;
	
}
char* DecString_to_BinString(char *DecString,int DecStringLen,char*BinString,int BinStringLen)
{
	char HexString[DecStringLen];
	char BinStringTmp[BinStringLen];
	sprintf(HexString,"%X",atoi(DecString));
	HexString_to_BinString(HexString,strlen(HexString),BinStringTmp);
	if(BinStringLen<strlen(BinStringTmp))
	{
		printf("error in DecString_to_BinString:BinStringLen < strlen(BinStringTmp) \n ");
	}
	memset(BinString,0x30,BinStringLen);
	strcpy(BinString+BinStringLen-strlen(BinStringTmp),BinStringTmp);
	//printf("%d %d\n",BinStringLen,strlen(BinStringTmp));
	printf("DecString_to_BinString:     DecString:%s  BinStringTmp %s BinString:%s \n",DecString,BinStringTmp,BinString);
	return BinString;
}

char* String_to_BinString(char *String,int len,char *BinString)
{
	char HexString[len*2];
	int i;
	for(i=0;i<strlen(String);i++)
	{
		sprintf(HexString+2*i,"%02x",*(unsigned char*)(String+i));
	}
	HexString_to_BinString(HexString,strlen(HexString),BinString);
	printf("String_to_BinString:     String:%s  BinString:%s \n",String,BinString);
	return BinString;
}
/*
void any_to_BinString(char *OrigString,int format,char *BinString,int BinStringLen)
{
	char tmp[strlen(string)*2+1];
	char bin_str_buf[
	int i;
	memset(tmp,0,sizeof(tmp));
	if(format==0)//normal string
	{
		for(i=0;i<strlen(string);i++)
		{
			sprintf(tmp+2*i,"%02x",*(unsigned char*)(string+i));
		}
	}
	if(format==16)
	{
		strcpy(tmp,string);
	}
	if(format==10)
	{
		sprintf(tmp,"%X",atoi(string));
	}

	printf("%s\n",tmp);

	
	
}
*/
/*
char* any_to_BinString(char* OrigString,char *BinString,int BinStringLen,int format)//Hex or Dex string to bin string
{
	int a;
	int i;
	char HexString[BinStringLen];
	char bin4[5];
	char *BinStringTmp;
	memset(BinStringTmp,0,sizeof(BinStringTmp));
	memset(HexString,0,sizeof(HexString));
	
	if(format==10)
	{
		sprintf(HexString,"%X",atoi(OrigString));
	}
	else if(format==0)//normal string
	{
		for(i=0;i<strlen(OrigString);i++)
		{
			sprintf(HexString+2*i,"%02x",*(unsigned char*)(string+i));
		}
	}
	else if(format==16)
	{
		strcpy(HexString,OrigString);
	}
	else
	{
		printf("format error in any_to_BinString\n");
		return NULL;
	}
	for(i=0;i<strlen(HexString);i++)
	{

		strcat(BinStringTmp,get_bin_from_HEX(HexString[i],bin4));
		//printf("char:%c bin:%s\n",HexString[i],bin);
	}
	if(BinStringLen==0)//didn't know how long it should be
	{
		BinStringLen=strlen(BinStringTmp);
	}
	BinString=malloc();
	memset(BinString,0x30,BinStringLen);
	if(BinStringLen<strlen(BinStringTmp))
	{
		printf("error in any_to_BinString:BinStringLen <strlen(BinStringTmp) \n ");
	}
	strcpy(BinString+BinStringLen-strlen(BinStringTmp),BinStringTmp);
	return BinString;
	
}
*/


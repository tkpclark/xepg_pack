#include "confile.h"

//---------------------------------------------------------------------------

char* getParameterValue(char *line) 
{
  if ( line == NULL ) return NULL;
  char *p = strchr(line, '=');
  if( p==NULL )
        return NULL;
  p++;
  if( *p != '\"' )
        return NULL;
  p++;
  char *p_end = strchr( p, '\"' );
  if( p_end == NULL )
        return NULL;
  *p_end = 0;
  return p;
}


char* getParameterName(char *line) 
{
  if ( line == NULL ) return NULL;
  char *p = strchr(line, '=');
  if( p==NULL ) return NULL;
  *p = 0;
  return line;
}

char* read_config(char *ConName,char* value)
{
  FILE *fp=NULL;
  char buffer[1024];
  char *ptr_ParameterName=NULL, *ptr_ParameterValue=NULL;
  memset(buffer, 0, sizeof(buffer));
  if((fp=fopen(CONFIG_FILE,"r"))==NULL) 
  {
	
  	return NULL;
  }
  while( fgets(buffer,180,fp)!=NULL )
  {
    if(  buffer[0]=='\0'
      || buffer[0]=='\n'
      || buffer[0]=='#'
      || buffer[0]=='%'
      || buffer[0]==';' ) continue;  
    if( buffer[(int)strlen(buffer)-1]==10 )
        buffer[strlen(buffer)-1]=0;
    ptr_ParameterValue = getParameterValue(buffer);
    
    ptr_ParameterName =  getParameterName(buffer);
    if(ptr_ParameterName==NULL)
    	continue;
    if( strcmp( ptr_ParameterName,ConName) == 0 ) 
    {
      strcpy(value,ptr_ParameterValue );
	fclose(fp);
      return value;
    }
    else//wrong para
    {
      continue;//read the next line
    }
  }
  fclose(fp);
  return NULL;
}



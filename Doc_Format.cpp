#include "Doc_Format.h"

void Doc_Format_Arg(char *file_name,int lf,const char *buf,va_list arg)
{
	int str;
	char buffer[500];
	char path[100];

	vsprintf(buffer,buf,arg);

	if(lf)
		  sprintf(buffer,"%s\n",buffer);

	sprintf(path,"./%s",file_name);	   
	if((str=open(path,O_RDWR|O_CREAT|O_APPEND, 0666))==-1)
	{
		return;
	}
	if(write(str,buffer,strlen(buffer))==-1)
	 {
	   close(str);
	   return;
	 }
	close(str);
}

#ifdef DEBUG
void Doc_Format(char *file_name,const char *buf,...)
{
	va_list arg;
	va_start(arg,buf);
	Doc_Format_Arg(file_name,1,buf,arg);
	va_end(arg);
}
#else
void Doc_Format(char *file_name,const char *buf,...){   }
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
          
void Doc_Format_Arg(char *file_name,int lf,const char *buf,va_list arg);
void Doc_Format(char *file_name,const char *buf,...);


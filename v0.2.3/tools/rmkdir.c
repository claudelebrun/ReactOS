#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef UNIX_PATHS
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#ifdef DOS_PATHS
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif	
#endif	

char* convert_path(char* origpath)
{
   char* newpath;
   int i;
   
   //newpath = strdup(origpath);
	 newpath=malloc(strlen(origpath)+1);
	 strcpy(newpath,origpath);
   
   i = 0;
   while (newpath[i] != 0)
     {
#ifdef UNIX_PATHS
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef DOS_PATHS
	if (newpath[i] == '/')
	  {
	     newpath[i] = '\\';
	  }
#endif	
#endif	
	i++;
     }
   return(newpath);
}

#define TRANSFER_SIZE      (65536)

int mkdir_p(char* path)
{
   if (chdir(path) == 0)
     {
	return(0);
     }
#ifdef UNIX_PATHS
   if (mkdir(path, 0755) != 0)
     {
	perror("Failed to create directory");
	exit(1);
     }
#else
   if (mkdir(path) != 0)
     {
	perror("Failed to create directory");
	exit(1);
     }
#endif
   
   if (chdir(path) != 0)
     {
	perror("Failed to change directory");
	exit(1);
     }
   return(0);
}

int main(int argc, char* argv[])
{
   char* path1;
   char* csec;
   char buf[256];
   
   if (argc != 2)
     {
	fprintf(stderr, "Too many arguments\n");
	exit(1);
     }
   
   path1 = convert_path(argv[1]);
   
   if (isalpha(path1[0]) && path1[1] == ':' && path1[2] == DIR_SEPARATOR_CHAR)
     {
	csec = strtok(path1, DIR_SEPARATOR_STRING);
  sprintf(buf, "%s\\", csec);
	chdir(buf);
	csec = strtok(NULL, DIR_SEPARATOR_STRING);
     }
   else if (path1[0] == DIR_SEPARATOR_CHAR)
     {
	chdir(DIR_SEPARATOR_STRING);
	csec = strtok(path1, DIR_SEPARATOR_STRING);
     }
   else
     {
	csec = strtok(path1, DIR_SEPARATOR_STRING);
     }
   
   while (csec != NULL)
     {
	mkdir_p(csec);
	csec = strtok(NULL, DIR_SEPARATOR_STRING);
     }
   
   exit(0);
}

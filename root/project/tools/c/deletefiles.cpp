
/**********************************************************************/
/* Author: Wangxiao                                        
/* Date  : 2023-05-25
/* Func  : This program is used to delete historical data files or log files.                                             
/**********************************************************************/
#include "_public.h"
//sigal handler for SIGINT and SIGTERM.
void EXIT(int sig);

int main(int argc,char *argv[])
{
  //helper of the program.
  if (argc != 4)
  {
    printf("\n");
    printf("Using:~/root/project/tools/bin/deletefiles pathname matchstr timeout\n\n");
    printf("Example:~/root/project/tools/bin/deletefiles ~/root/log/idc \"*.log.20*\" 0.02\n");
    printf("        ~/root/project/tools/bin/deletefiles ~/root/tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
    printf("        ~/root/project/tools/bin/procctl 300 ~/root/project/tools/bin/deletefiles ~/root/log/idc \"*.log.20*\" 0.02\n");
    printf("        ~/root/project/tools/bin/procctl 300 ~/root/project/tools/bin/deletefiles ~/root/tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");
    printf("This is a utility program used to compress historical data files or log files.\n");
    printf("This program delete all files in the pathname directory and its subdirectories that match matchstr and are older than timeout days. The timeout can be a decimal.\n");
    printf("This program does not write to log files, nor does it output any information to the console.\n");
    return -1;
  }

 // Close all signals and input/output.
  // Set the signal, under shell state "kill + process number" can normally terminate this process.
  // But please do not use "kill -9 +process number" to forcefully terminate.
  signal(SIGINT,EXIT);  signal(SIGTERM,EXIT);

  // Get the timeout time.
  char strTimeOut[21];
  LocalTime(strTimeOut,"yyyy-mm-dd hh24:mi:ss",0-(int)(atof(argv[3])*24*60*60));

  CDir Dir;
  // Open the directory.
  // open the directory argv[1] is the file we want to explore, argv[2] is the file name we want to match, 10000 is the max file number in the directory.
  if (Dir.OpenDir(argv[1],argv[2],10000,true)==false)
  {
    printf("Dir.OpenDir(%s) failed.\n",argv[1]); return -1;
  }
  //traverse the directory.
  while (true)
  {
    // get the next file. 
    if (Dir.ReadDir()==false) break;
    //printf("=%s=\n",Dir.m_FullFileName);  // print the file name.
    //printf("=%s=\n",Dir.m_ModifyTime);   // print the file modify time.
    //printf("=%s=\n",strTimeOut);         // print the timeout time.
    //printf("=%d=\n",strcmp(Dir.m_ModifyTime,strTimeOut)<0); // print the compare result.
    if (strcmp(Dir.m_ModifyTime,strTimeOut)<0) 
    {
     
      if (REMOVE(Dir.m_FullFileName)==0) 
        printf("REMOVE %s ok.\n",Dir.m_FullFileName);
      else
        printf("REMOVE %s failed.\n",Dir.m_FullFileName);
    }
  }

  return 0;
}

void EXIT(int sig)
{
  printf("deletefiles exit...，sig=%d\n\n",sig);

  exit(0);
}

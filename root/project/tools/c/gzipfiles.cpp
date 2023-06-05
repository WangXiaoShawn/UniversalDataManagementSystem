#include "_public.h"
/**********************************************************************/
/* Author: Wangxiao                                        
/* Date  : 2023-05-25
/* Func  : This program is used to compress historical data files or log files.                                             
/**********************************************************************/

// The signal handler for SIGINT and SIGTERM.
void EXIT(int sig);

int main(int argc,char *argv[])
{

  if (argc != 4)
  {
    printf("\n");
    printf("Using:~/root/project/tools/bin/gzipfiles pathname matchstr timeout\n\n");

    printf("Example:~/root/project/tools/bin/gzipfiles ~/root/log/idc \"*.log.20*\" 0.02\n");
    printf("        ~/root/project/tools/bin/gzipfiles ~/root/tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
    printf("        ~/root/project/tools/bin/procctl 300 ~/root/project/tools/bin/gzipfiles ~/root/log/idc \"*.log.20*\" 0.02\n");
    printf("        ~/root/project/tools/bin/procctl 300 ~/root/project/tools/bin/gzipfiles ~/root/tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

    printf("This is a utility program used to compress historical data files or log files.\n");
    printf("This program compresses all files in the pathname directory and its subdirectories that match matchstr and are older than timeout days. The timeout can be a decimal.\n");
    printf("This program does not write to log files, nor does it output any information to the console.\n");
    printf("This program calls the /usr/bin/gzip command to compress files.\n\n\n");
    return -1;
  }

  // Close all signals and input/output.
  // Set the signal, under shell state "kill + process number" can normally terminate this process.
  // But please do not use "kill -9 +process number" to forcefully terminate.
  CloseIOAndSignal(true); 
  signal(SIGINT,EXIT);  
  signal(SIGTERM,EXIT);

  // get the time out point
  char strTimeOut[21];
  LocalTime(strTimeOut,"yyyy-mm-dd hh24:mi:ss",0-(int)(atof(argv[3])*24*60*60)); // use local time to calculate the time out point
  CDir Dir; //CDirectory object is used to traverse the directory and do the file operations.
  // open the directory argv[1] is the file we want to explore, argv[2] is the file name we want to match, 10000 is the max file number in the directory, true means we want to explore the subdirectory.
  if (Dir.OpenDir(argv[1],argv[2],10000,true)==false)
  {
    printf("Dir.OpenDir(%s) failed.\n",argv[1]); return -1;
  }

  char strCmd[1024]; // strCmd is used to store the command we want to execute.

  // traverse the directory
  while (true)
  {
    // get the file direction if return to false, it means there is no file in the directory.
    if (Dir.ReadDir()==false) break;
  
    // check the time of the file, if the file is older than the time out point, we compress it and ensure the file is not a compressed file.
    if ( (strcmp(Dir.m_ModifyTime,strTimeOut)<0) && (MatchStr(Dir.m_FileName,"*.gz")==false) )
    {
      // command to compress the file
      SNPRINTF(strCmd,sizeof(strCmd),1000,"/usr/bin/gzip -f %s 1>/dev/null 2>/dev/null",Dir.m_FullFileName); // compress the file
      if (system(strCmd)==0) 
        printf("gzip %s ok.\n",Dir.m_FullFileName);
      else
        printf("gzip %s failed.\n",Dir.m_FullFileName);
    }
  }

  return 0;
}

void EXIT(int sig)
{
  printf("gzipfiles exit...，sig=%d\n\n",sig);

  exit(0);
}

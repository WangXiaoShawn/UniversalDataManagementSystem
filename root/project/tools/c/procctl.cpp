/**********************************************************************/
/* Author: Wangxiao                                        
/* Date  : 2023-05-25
/* Func  : This program is the scheduler for service programs, periodically launching service programs or shell scripts.     
/* Email : wangxiaojobhunting@gmail.com                                   
/**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc,char *argv[])
{
  if (argc<3)
  {
    printf("\n");
    printf("Usage: ./procctl timetvl program argv ...\n");
    printf("Example: ~/root/project/tools/bin/procctl 5 /usr/bin/tar zcvf /tmp/tmp.tgz /usr/include\n\n");
    printf("This program is the scheduler for service programs, periodically launching service programs or shell scripts.\n");
    printf("timetvl is the operation cycle, in seconds. After the scheduled program finishes running, it will be restarted by procctl after timetvl seconds.\n");
    printf("program is the name of the scheduled program, the full path must be used.\n");
    printf("argvs are the parameters of the scheduled program.\n");
    printf("Note, this program will not be killed by kill, but can be forcibly killed with kill -9.\n\n\n");
    return -1;
  }
  //trun off the signal and IO, this program will not be distrub
  for (int ii=0;ii<64;ii++)
  {
    signal(ii,SIG_IGN); close(ii); // signal 0-64 all ignore SIg_ING 1-64 and close file discripter 0-64 ;
  }

  // generate child process, then exit the parent process, then the child process will be handled by init process
  if (fork()!=0) exit(0); 


  // When I (the process) receive a SIGCHLD signal (from one of my child processes), I will take the default action
  signal(SIGCHLD,SIG_DFL);

  char *pargv[argc]; //create a new argv for execv
  for (int ii=2;ii<argc;ii++)
    pargv[ii-2]=argv[ii];//copy argv to pargv for only two argvs

  pargv[argc-2]=NULL; //end of pargv give A NULL as end of argv signal

  while (true)
  {
    if (fork()==0) //child 
    {
      execv(argv[2],pargv);
      exit(0);
    }
    else
    {
      int status;
      wait(&status);
      sleep(atoi(argv[1]));
    }
  }
}

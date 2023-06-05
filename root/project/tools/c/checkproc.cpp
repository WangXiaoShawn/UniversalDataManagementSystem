/**********************************************************************/
/* Author: Wangxiao                                        
/* Date  : 2023-05-25
/* Func  : This program is used to check whether the background service program has timed out. If it has, it terminates it. 
/* The operation will be write into log file.
/* Email: wangxiaojobhunting@gmail.com                                         
/**********************************************************************/
#include "_public.h"
CLogFile logfile;

int main(int argc,char *argv[])
{
  // 程序的帮助。
  if (argc != 2)
  {
    printf("\n");
    printf("Usage: ./checkproc logfilename\n");
    printf("Example: ~/root/project/tools/bin/procctl 10 ~/root/project/tools/bin/checkproc ~/root/tmp/log/checkproc.log\n\n");
    printf("This program is used to check whether the background service program has timed out. If it has, it terminates it.\n");
    printf("Note:\n");
    printf(" 1) This program is launched by procctl, and the recommended running cycle is 10 seconds.\n");
    printf(" 2) To avoid being accidentally terminated by regular users, this program should be started with the root user.\n");
    printf(" 3) If you want to stop this program, you can only use killall -9 to terminate it.\n\n\n");

    return 0;
  }

  // neglet the signal and IO to avoid disturb.
  CloseIOAndSignal(true);


  if (logfile.Open(argv[1],"a+")==false)  /* ~/root/tmp/log/checkproc.log */
  { printf("logfile.Open(%s) failed.\n",argv[1]); return -1; }

  int shmid=0;
  // get the shared memory, the key is SHMKEYP, the size is MAXNUMP*sizeof(struct st_procinfo).
  if ( (shmid = shmget((key_t)SHMKEYP, MAXNUMP*sizeof(struct st_procinfo), 0666|IPC_CREAT)) == -1)
  {
    logfile.Write("Failed to create/get shared memory (%x).\n", SHMKEYP); return false;
  }

  // link the shared memory to the current process's memory space.
  struct st_procinfo *shm=(struct st_procinfo *)shmat(shmid, 0, 0); 
  //interate all the records in the shared memory.
  for (int ii=0;ii<MAXNUMP;ii++)
  {
    // if the pid is 0, it means the record is empty, continue.
    if (shm[ii].pid==0) continue; //shm[ii] is (struct st_procinfo *)(shm+ii) is the structure
    // if the pid is not 0, it means the record is the heartbeat record of the service program.

    // 程序稳定运行后，以下两行代码可以注释掉。
    //logfile.Write("ii=%d,pid=%d,pname=%s,timeout=%d,atime=%d\n",\
    //               ii,shm[ii].pid,shm[ii].pname,shm[ii].timeout,shm[ii].atime);

    // send the signal 0 to the process, if the process does not exist, delete the record from the shared memory.
    int iret=kill(shm[ii].pid,0); // signal 0 is used to test if the process exists
    if (iret==-1)
    {
      logfile.Write("The process pid=%d(%s) no longer exists.\n", (shm+ii)->pid, (shm+ii)->pname);
      memset(shm+ii, 0, sizeof(struct st_procinfo)); // Delete the record from shared memory.
      continue;
    }

    time_t now=time(0);   // get the current time.

    // if the process has not timed out, continue.
    if (now-shm[ii].atime<shm[ii].timeout) continue;

    // if the process has timed out, write the log.
    logfile.Write("The process pid=%d(%s) has timed out.\n", (shm+ii)->pid, (shm+ii)->pname);
    // send signal 15 to the process, try to terminate it normally.
    kill(shm[ii].pid,15);     
    // send signal 0 to the process every 1 second to check if it still exists, accumulate 5 seconds, generally, 5 seconds is enough for the process to exit.
    for (int jj=0;jj<5;jj++)
    {
      sleep(1);
      iret=kill(shm[ii].pid,0);     
      if (iret==-1) break;     // if the process does not exist, break.
    } 

    // if the process still exists, send signal 9 to terminate it.
    if (iret==-1)
    {
      logfile.Write("The process pid=%d(%s) has terminated normally.\n", (shm+ii)->pid, (shm+ii)->pname);
    }
    else
    {
      kill(shm[ii].pid,9);  // if the process still exists, send signal 9 to terminate it.
      logfile.Write("The process pid=%d(%s) has been forcibly terminated.\n", (shm+ii)->pid, (shm+ii)->pname);
    }
    
    // delete the record from the shared memory.
    memset(shm+ii,0,sizeof(struct st_procinfo)); 
  }

  // unlink the shared memory from the current process's memory space.
  shmdt(shm);

  return 0;
}

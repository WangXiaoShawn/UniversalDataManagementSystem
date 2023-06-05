
/*
 * Program Name: obtmindtodb.cpp This program is used to import national site minute observation data 
 *  into the T_ZHOBTMIND table, supporting both xml and csv file formats.
 * Author: WangXiao
 * Email: WANGXIAOJOBHUNTING @GMAIL.COM
 * Date: 2023/6/4
*/
#include "idcapp.h"

CLogFile logfile;

connection conn;

CPActive PActive;

void EXIT(int sig);

bool _obtmindtodb(char *pathname,char *connstr,char *charset);

int main(int argc,char *argv[])
{
  // helper function 
  if (argc!=5)
  {
    printf("\n");
    printf("Using:./obtmindtodb pathname connstr charset logfile\n");

    printf("Example:/project/tools1/bin/procctl 10 ~/root/project/idc/bin/obtmindtodb ~/root/idcdata/surfdata \"127.0.0.1,root,cis623,mysql,3306\" utf8 ~/root/project/log/idc/obtmindtodb.log\n\n");

    printf("本程序用于把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新。\n");
    printf("pathname 全国站点分钟观测数据文件存放的目录。\n");
    printf("connstr  数据库连接参数：ip,username,password,dbname,port\n");
    printf("charset  数据库的字符集。\n");
    printf("logfile  本程序运行的日志文件名。\n");
    printf("程序每10秒运行一次，由procctl调度。\n\n\n");

    printf("Example:/project/tools1/bin/procctl 10 ~/root/project/idc/bin/obtmindtodb ~/root/idcdata/surfdata \"127.0.0.1,root,cis623,mysql,3306\" utf8 ~/root/log/idc/obtmindtodb.log\n\n");
    printf("This program is used to save national site minute observation data to the T_ZHOBTMIND table in the database. Data is only inserted, not updated.\n");
    printf("pathname is the directory where the national site minute observation data file is stored.\n");
    printf("connstr are the database connection parameters: ip, username, password, dbname, port\n");
    printf("charset is the character set of the database.\n");
    printf("logfile is the log file name of this program's run.\n");
    printf("The program runs every 10 seconds, scheduled by procctl.\n\n\n");

    return -1;
  }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  // 打开日志文件。
  if (logfile.Open(argv[4],"a+")==false)
  {
    printf("fail to open logfile （%s）。\n",argv[4]); return -1;
  }

  PActive.AddPInfo(30,"obtmindtodb");   // 进程的心跳，30秒足够。
  // 注意，在调试程序的时候，可以启用类似以下的代码，防止超时。
  // PActive.AddPInfo(5000,"obtmindtodb");

  // 业务处理主函数。
  _obtmindtodb(argv[1],argv[2],argv[3]);

  return 0;
}

void EXIT(int sig)
{
  logfile.Write("Program end，sig=%d\n\n",sig);

  conn.disconnect();

  exit(0);
}

// 业务处理主函数。
bool _obtmindtodb(char *pathname,char *connstr,char *charset)
{
  CDir Dir;

  // 打开目录。
  if (Dir.OpenDir(pathname,"*.xml,*.csv")==false)// open direction 
  {
    logfile.Write("Dir.OpenDir(%s) failed.\n",pathname); return false;
  }

  CFile File;

  CZHOBTMIND ZHOBTMIND(&conn,&logfile);

  int  totalcount=0;  // 文件的总记录数。// total record number of the file
  int  insertcount=0; // 成功插入记录数。// successfully inserted record number
  CTimer Timer;       // 计时器，记录每个数据文件的处理耗时。// timer, record the time consumption of each data file
  bool bisxml=false;  // 文件格式，true-xml；false-csv。// file format, true-xml; false-csv

  while (true)
  {
    // 读取目录，得到一个数据文件名。
    if (Dir.ReadDir()==false) break; // read direction if failed, break

    if (MatchStr(Dir.m_FullFileName,"*.xml")==true) bisxml=true; // if the file format is xml, bisxml=true
    else bisxml=false;

    // 连接数据库。
    if (conn.m_state==0) // if the database is not connected then connect
    {
      if (conn.connecttodb(connstr,charset)!=0)
      {
        logfile.Write("connect database(%s) failed.\n%s\n",connstr,conn.m_cda.message); return -1;
      }
    
      logfile.Write("connect database(%s) ok.\n",connstr);
    }

    totalcount=insertcount=0;

    // 打开文件。
    if (File.Open(Dir.m_FullFileName,"r")==false)
    {
      logfile.Write("File.Open(%s) failed.\n",Dir.m_FullFileName); return false;
    }

    char strBuffer[1001];   // 存放从文件中读取的一行。

    while (true)
    {
      if (bisxml==true)
      {
        if (File.FFGETS(strBuffer,1000,"<endl/>")==false) break;
      }
      else
      {
        if (File.Fgets(strBuffer,1000,true)==false) break;
        if (strstr(strBuffer,"站点")!=0) continue;   // 跳过文件中的第一行。// skip the first line of the file
      }

      // 处理文件中的每一行。
      totalcount++;

      ZHOBTMIND.SplitBuffer(strBuffer,bisxml);  

      if (ZHOBTMIND.InsertTable()==true) insertcount++;
    }

    // 删除文件、提交事务。
    File.CloseAndRemove();

    conn.commit();

    logfile.Write("already processed%s（totalcount=%d,insertcount=%d），耗时%.2f秒。\n",Dir.m_FullFileName,totalcount,insertcount,Timer.Elapsed());
  }

  return true;
}





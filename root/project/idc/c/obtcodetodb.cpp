/*
 * Program Name: obtcodetodb.cpp
 * Author: WangXiao
 * Email: WANGXIAOJOBHUNTING @GMAIL.COM
 * Date: 2023/6/4
*/

#include "_public.h"
#include "_mysql.h"

struct st_stcode
{
  char provname[31]; // 省 // Province
  char obtid[11];    // 站号 // Station number
  char cityname[31];  // 站名// Station name
  char lat[11];      // 纬度// Latitude
  char lon[11];      // 经度// Longitude
  char height[11];   // 海拔高度// Altitude
};

vector<struct st_stcode> vstcode; // 存放全国气象站点参数的容器。// container for the weather station infomation

// 把站点参数文件中加载到vstcode容器中。 // read  the ini file and load the stcode into the vector vstcode
bool LoadSTCode(const char *inifile);

CLogFile logfile; // 日志文件。// save operation to log file

connection conn; // 数据库连接。// database connection

CPActive PActive; // 进程的心跳。// Heartbeat of the process

void EXIT(int sig);

int main(int argc,char *argv[])
{
  // 帮助文档。
  if (argc!=5)
  {
    printf("\n");
    printf("Using:./obtcodetodb inifile connstr charset logfile\n");

    printf("Example:~/root/project/tools/bin/procctl 120 ~/root/project/idc/bin/obtcodetodb ~/root/project/idc/ini/stcode.ini \"127.0.0.1,root,cis623,mysql,3306\" utf8 /log/idc/obtcodetodb.log\n\n");

    printf("本程序用于把全国站点参数数据保存到数据库表中，如果站点不存在则插入，站点已存在则更新。\n");
    printf("inifile 站点参数文件名（全路径）。\n");
    printf("connstr 数据库连接参数：ip,username,password,dbname,port\n");
    printf("charset 数据库的字符集。\n");
    printf("logfile 本程序运行的日志文件名。\n");
    printf("程序每120秒运行一次，由procctl调度。\n\n\n");
    printf("\n");
    printf("Usage: ./obtcodetodb inifile connstr charset logfile\n");
    printf("Example: ~/root/project/tools/bin/procctl 120 ~/root/project/idc/bin/obtcodetodb ~/root/project/idc/ini/stcode.ini \"127.0.0.1,root,cis623,mysql,3306\" utf8 ~/root/log/idc/obtcodetodb.log\n\n");
    printf("This program is used to save national site parameter data to the database table. If the site does not exist, it is inserted. If the site already exists, it is updated.\n");
    printf("inifile is the full path to the site parameter file.\n");
    printf("connstr are the database connection parameters: ip, username, password, dbname, port\n");
    printf("charset is the character set of the database.\n");
    printf("logfile is the log file name of this program's run.\n");
    printf("The program runs every 120 seconds, scheduled by procctl.\n\n\n");
    return -1;
  }

  
  CloseIOAndSignal(); // turn off all the signal and input and output
  signal(SIGINT,EXIT); // set the signal, in the shell state, you can use "kill + process number" to terminate the process normally.
  signal(SIGTERM,EXIT);// but please do not use "kill -9 + process number" to terminate it forcibly.

  // 打开日志文件。
  if (logfile.Open(argv[4],"a+")==false)
  {
    printf("fail to open logfile(%s)。\n",argv[4]); return -1;
  }

  PActive.AddPInfo(10,"obtcodetodb");   //add the process information to the PActive
  // 注意，在调试程序的时候，可以启用类似以下的代码，防止超时。
  // PActive.AddPInfo(5000,"obtcodetodb");

  // 把全国站点参数文件加载到vstcode容器中。  
  if (LoadSTCode(argv[1])==false) return -1; // load the stcode into the vector vstcode

  logfile.Write("Load the ini file (%s) sucessful, number of (%d) record。\n",argv[1],vstcode.size());

  // 连接数据库。
  if (conn.connecttodb(argv[2],argv[3])!=0)
  {
    logfile.Write("connect database(%s) failed.\n%s\n",argv[2],conn.m_cda.message); return -1;
  }

  logfile.Write("connect database(%s) ok.\n",argv[2]);

  struct st_stcode stcode;

  // 准备插入表的SQL语句。
  //prepare -bindin -execute -commit
  sqlstatement stmtins(&conn);
  stmtins.prepare("insert into T_ZHOBTCODE(obtid,cityname,provname,lat,lon,height,upttime) values(:1,:2,:3,:4*100,:5*100,:6*10,now())");
  stmtins.bindin(1,stcode.obtid,10);
  stmtins.bindin(2,stcode.cityname,30);
  stmtins.bindin(3,stcode.provname,30);
  stmtins.bindin(4,stcode.lat,10);
  stmtins.bindin(5,stcode.lon,10);
  stmtins.bindin(6,stcode.height,10);

  // 准备更新表的SQL语句。// prepare the sql statement to update the table
  sqlstatement stmtupt(&conn);
  stmtupt.prepare("update T_ZHOBTCODE set cityname=:1,provname=:2,lat=:3*100,lon=:4*100,height=:5*10,upttime=now() where obtid=:6");
  stmtupt.bindin(1,stcode.cityname,30); // loc,value ,len
  stmtupt.bindin(2,stcode.provname,30);
  stmtupt.bindin(3,stcode.lat,10);
  stmtupt.bindin(4,stcode.lon,10);
  stmtupt.bindin(5,stcode.height,10);
  stmtupt.bindin(6,stcode.obtid,10);

  int inscount=0,uptcount=0;
  CTimer Timer;

  
  for (int ii=0;ii<vstcode.size();ii++)
  {
    memcpy(&stcode,&vstcode[ii],sizeof(struct st_stcode)); // already bindin, so just copy the data to the stcode

    // 执行插入的SQL语句。
    if (stmtins.execute()!=0)
    {
      if (stmtins.m_cda.rc==1062)// 1062: Duplicate entry '500000' for key 'PRIMARY'
      {
        // 如果记录已存在，执行更新的SQL语句。
        if (stmtupt.execute()!=0) 
        {
          logfile.Write("stmtupt.execute() failed.\n%s\n%s\n",stmtupt.m_sql,stmtupt.m_cda.message); return -1;
        }
        else
          uptcount++;
      }
      else
      {
        
        logfile.Write("stmtins.execute() failed.\n%s\n%s\n",stmtins.m_sql,stmtins.m_cda.message); return -1;
      }
    }
    else
      inscount++;
  }
  
  
  logfile.Write("total record=%d, insert=%d, update=%d, time =%.2fs。\n",vstcode.size(),inscount,uptcount,Timer.Elapsed());

  
  conn.commit();

  return 0;
}

// 把站点参数文件中加载到vstcode容器中。
bool LoadSTCode(const char *inifile)
{
  CFile File;

  // 打开站点参数文件。
  if (File.Open(inifile,"r")==false)
  {
    logfile.Write("File.Open(%s) failed.\n",inifile); return false;
  }

  char strBuffer[301];

  CCmdStr CmdStr;

  struct st_stcode stcode;

  while (true)
  {
    
    if (File.Fgets(strBuffer,300,true)==false) break; // read one line from the file

   
    CmdStr.SplitToCmd(strBuffer,",",true); //split the line to the CmdStr

    if (CmdStr.CmdCount()!=6) continue;     // a valid line must have 6 items

    // 把站点参数的每个数据项保存到站点参数结构体中。
    memset(&stcode,0,sizeof(struct st_stcode));
    CmdStr.GetValue(0, stcode.provname,30); // 省 // get the value of the first item it is province name
    CmdStr.GetValue(1, stcode.obtid,10);    // 站号 // get the value of the second item it is station id
    CmdStr.GetValue(2, stcode.cityname,30);  // 站名 // get the value of the third item it is station name
    CmdStr.GetValue(3, stcode.lat,10);      // 纬度 // get the value of the fourth item it is latitude
    CmdStr.GetValue(4, stcode.lon,10);      // 经度 // get the value of the fifth item it is longitude
    CmdStr.GetValue(5, stcode.height,10);   // 海拔高度 // get the value of the sixth item it is height

    // 把站点参数结构体放入站点参数容器。
    vstcode.push_back(stcode);
  }



  return true;
}

void EXIT(int sig)
{
  logfile.Write("program end，sig=%d\n\n",sig);

  conn.disconnect();

  exit(0);
}

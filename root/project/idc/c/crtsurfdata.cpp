/*

    Program Name: crtsurfdata.cpp This program is used to generate minute-by-minute observational data for national meteorological stations.
    Author: WangXiao
    Email: WANGXIAOJOBHUNTING @GMAIL.COM
    Date: 2023/5/25
    */

#include "_public.h"

CPActive PActive;   // Heartbeat of the process

struct st_stcode //st_stcode is the structure save the information of the weather station

{
char provname[31]; // Province
char obtid[11]; // Station number
char obtname[31]; // Station name
double lat; // Latitude
double lon; // Longitude
double height; // Altitude
};

vector<struct st_stcode> vstcode; // container for the weather station infomation

// read  the ini file and load the stcode into the vector vstcode
bool LoadSTCode(const char *inifile); 

struct st_surfdata // for each weather station, there is a structure to save the weather data
{
char obtid[11]; // Station code.
char ddatetime[21]; // Data time: format yyyymmddhh24miss
int t; // Temperature: unit, 0.1 degrees Celsius.
int p; // Pressure: 0.1 hPa.
int u; // Relative humidity, a value between 0-100.
int wd; // Wind direction, a value between 0-360.
int wf; // Wind speed: unit 0.1m/s
int r; // Rainfall: 0.1mm.
int vis; // Visibility: 0.1 meter.
};

vector<struct st_surfdata> vsurfdata;  // container for the weather data
char strddatetime[21]; // the datetime of the weather data

void CrtSurfData(); //stimulate the weather data for each minute, and save the data into the container vsurfdata

CFile File;  // save the data into the file

bool CrtSurfFile(const char *outpath,const char *datafmt); // write the weather data into the file

CLogFile logfile;    // save operation to log file

void EXIT(int sig);  // function to handle the exit signal

/****************************************************************************************************************************************************/

int main(int argc,char *argv[])
{
  if ( (argc!=5) && (argc!=6) ) // add a parameter for user defined datetime
  {
    // If the parameter is illegal, give the help document.
    printf("Using:./crtsurfdata inifile outpath logfile datafmt [datetime]\n");
    printf("Example:~/root/project/idc/bin/crtsurfdata ~/root/project/idc/ini/stcode.ini ~/root/tmp/idc/surfdata ~/root/log/idc/crtsurfdata.log xml,json,csv\n");
    printf("        ~/root/project/idc/bin/crtsurfdata ~/root/project/idc/ini/stcode.ini ~/root/idc/surfdata ~/root/log/idc/crtsurfdata.log xml,json,csv 20210710123000\n");
    printf("        ~/root/project/tools/bin/procctl 60 ~/root/project/idc/bin/crtsurfdata ~/root/project/idc/ini/stcode.ini ~/root/idc/surfdata ~/root/log/idc/crtsurfdata.log xml,json,csv\n\n\n");
    printf("inifile Name of the national meteorological station parameter file.\n");
    printf("outpath Directory where the national meteorological station data files are stored.\n");
    printf("logfile The log file name of this program's execution.\n");
    printf("datafmt The format of the data file to be generated, supports xml, json, and csv, separated by commas.\n");
    printf("datetime This is an optional parameter, indicating the generation of data and files at a specified time.\n\n\n");
    return -1;
  }

// Close all signals and input/output.
// Set signal, this process can be terminated normally with "kill + process number" in shell status.
// But please do not use "kill -9 + process number" to forcibly terminate.
// The signal setting must be before the log opening, otherwise the log file will be truncated(closeIO and signal)
  CloseIOAndSignal(true); 
  signal(SIGINT,EXIT);   // ctrl+c
  signal(SIGTERM,EXIT); // kill pid

  if (logfile.Open(argv[3],"a+",false)==false) // open the log file if failed, return -1 a+ is append and read and write
  {
    printf("logfile.Open(%s) failed.\n",argv[3]); return -1;
  }

  logfile.Write("crtsurfdata start running... \n");

  PActive.AddPInfo(20,"crtsurfdata"); // settime out is 20s(i) process name is crtsurfdata and since logfile pointer is null the operation will not be write to log file
  if (LoadSTCode(argv[1])==false) return -1; // no ini return -1

  // get the current time or you can set the time by yourself
  memset(strddatetime,0,sizeof(strddatetime));
  if (argc==5)
    LocalTime(strddatetime,"yyyymmddhh24miss");
  else
    STRCPY(strddatetime,sizeof(strddatetime),argv[5]); // set the time by yourself

  // sitimulate the weather data for each minute, and save the data into the container vsurfdata
  CrtSurfData();

  // write the weather data into the file
  if (strstr(argv[4],"xml")!=0) CrtSurfFile(argv[2],"xml");
  if (strstr(argv[4],"json")!=0) CrtSurfFile(argv[2],"json");
  if (strstr(argv[4],"csv")!=0) CrtSurfFile(argv[2],"csv");

  logfile.Write("crtsurfdata end...\n");

  return 0;
}

/****************************************************************************************************************************************************/

// 把站点参数文件中加载到vstcode容器中。 
bool LoadSTCode(const char *inifile)
{
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
    // 从站点参数文件中读取一行，如果已读取完，跳出循环。
    if (File.Fgets(strBuffer,300,true)==false) break;

    // 把读取到的一行拆分。
    CmdStr.SplitToCmd(strBuffer,",",true);

    if (CmdStr.CmdCount()!=6) continue;     // 扔掉无效的行。

    // 把站点参数的每个数据项保存到站点参数结构体中。
    memset(&stcode,0,sizeof(struct st_stcode));
    CmdStr.GetValue(0, stcode.provname,30); // 省
    CmdStr.GetValue(1, stcode.obtid,10);    // 站号
    CmdStr.GetValue(2, stcode.obtname,30);  // 站名
    CmdStr.GetValue(3,&stcode.lat);         // 纬度
    CmdStr.GetValue(4,&stcode.lon);         // 经度
    CmdStr.GetValue(5,&stcode.height);      // 海拔高度

    // 把站点参数结构体放入站点参数容器。
    vstcode.push_back(stcode);
  }

  /*
  for (int ii=0;ii<vstcode.size();ii++)
    logfile.Write("provname=%s,obtid=%s,obtname=%s,lat=%.2f,lon=%.2f,height=%.2f\n",\
                   vstcode[ii].provname,vstcode[ii].obtid,vstcode[ii].obtname,vstcode[ii].lat,\
                   vstcode[ii].lon,vstcode[ii].height);
  */

  return true;
}

// 模拟生成全国气象站点分钟观测数据，存放在vsurfdata容器中。
void CrtSurfData()
{
  // 播随机数种子。
  srand(time(0));

  struct st_surfdata stsurfdata;

  // 遍历气象站点参数的vstcode容器。
  for (int ii=0;ii<vstcode.size();ii++)
  {
    memset(&stsurfdata,0,sizeof(struct st_surfdata));

    // 用随机数填充分钟观测数据的结构体。
    strncpy(stsurfdata.obtid,vstcode[ii].obtid,10); // 站点代码。
    strncpy(stsurfdata.ddatetime,strddatetime,14);  // 数据时间：格式yyyymmddhh24miss
    stsurfdata.t=rand()%351;       // 气温：单位，0.1摄氏度
    stsurfdata.p=rand()%265+10000; // 气压：0.1百帕
    stsurfdata.u=rand()%100+1;     // 相对湿度，0-100之间的值。
    stsurfdata.wd=rand()%360;      // 风向，0-360之间的值。
    stsurfdata.wf=rand()%150;      // 风速：单位0.1m/s
    stsurfdata.r=rand()%16;        // 降雨量：0.1mm
    stsurfdata.vis=rand()%5001+100000;  // 能见度：0.1米

    // 把观测数据的结构体放入vsurfdata容器。
    vsurfdata.push_back(stsurfdata);
  }
}

// 把容器vsurfdata中的全国气象站点分钟观测数据写入文件。
bool CrtSurfFile(const char *outpath,const char *datafmt)
{
  // 拼接生成数据的文件名，例如：~/root/tmp/idc/surfdata/SURF_ZH_20210629092200_2254.csv
  char strFileName[301];
  sprintf(strFileName,"%s/SURF_ZH_%s_%d.%s",outpath,strddatetime,getpid(),datafmt);

  // 打开文件。
  if (File.OpenForRename(strFileName,"w")==false)
  {
    logfile.Write("File.OpenForRename(%s) failed.\n",strFileName); return false;
  }

  if (strcmp(datafmt,"csv")==0) File.Fprintf("站点代码,数据时间,气温,气压,相对湿度,风向,风速,降雨量,能见度\n");
  if (strcmp(datafmt,"xml")==0) File.Fprintf("<data>\n");
  if (strcmp(datafmt,"json")==0) File.Fprintf("{\"data\":[\n");

  // 遍历存放观测数据的vsurfdata容器。 // the current file is temp file ,not the final file
  // this design is to guarantee the write process is automic
  for (int ii=0;ii<vsurfdata.size();ii++)
  {
    // 写入一条记录。
    if (strcmp(datafmt,"csv")==0)
      File.Fprintf("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",\
         vsurfdata[ii].obtid,vsurfdata[ii].ddatetime,vsurfdata[ii].t/10.0,vsurfdata[ii].p/10.0,\
         vsurfdata[ii].u,vsurfdata[ii].wd,vsurfdata[ii].wf/10.0,vsurfdata[ii].r/10.0,vsurfdata[ii].vis/10.0);

    if (strcmp(datafmt,"xml")==0)
      File.Fprintf("<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</p>"\
                   "<u>%d</u><wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n",\
         vsurfdata[ii].obtid,vsurfdata[ii].ddatetime,vsurfdata[ii].t/10.0,vsurfdata[ii].p/10.0,\
         vsurfdata[ii].u,vsurfdata[ii].wd,vsurfdata[ii].wf/10.0,vsurfdata[ii].r/10.0,vsurfdata[ii].vis/10.0);

    if (strcmp(datafmt,"json")==0)
    {
      File.Fprintf("{\"obtid\":\"%s\",\"ddatetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%.1f\","\
                   "\"u\":\"%d\",\"wd\":\"%d\",\"wf\":\"%.1f\",\"r\":\"%.1f\",\"vis\":\"%.1f\"}",\
         vsurfdata[ii].obtid,vsurfdata[ii].ddatetime,vsurfdata[ii].t/10.0,vsurfdata[ii].p/10.0,\
         vsurfdata[ii].u,vsurfdata[ii].wd,vsurfdata[ii].wf/10.0,vsurfdata[ii].r/10.0,vsurfdata[ii].vis/10.0);

      if (ii<vsurfdata.size()-1) File.Fprintf(",\n");
      else   File.Fprintf("\n");
    }
  }

  if (strcmp(datafmt,"xml")==0) File.Fprintf("</data>\n");
  if (strcmp(datafmt,"json")==0) File.Fprintf("]}\n");

  // 关闭文件。
  File.CloseAndRename(); // 关闭文件，并重命名为正式文件名。

  UTime(strFileName,strddatetime);  // 修改文件的时间属性。
  logfile.Write("Successfully created data file %s, data time %s, record count %d.\n", strFileName, strddatetime, vsurfdata.size());

  return true;
}

// 程序退出和信号2、15的处理函数。
void EXIT(int sig)  
{
  logfile.Write("crtsurfdata ending...，sig=%d\n\n",sig);

  exit(0);
}

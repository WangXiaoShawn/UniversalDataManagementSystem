
/*
 * Program Name: dminingmysql.cpp, this program is a common function module of the data center, used to extract data from the MySQL database source table 
 * and generate xml files.
 * Author: WangXiao
 * Email: WANGXIAOJOBHUNTING @GMAIL.COM
 * Date: 2023/6/4
*/
#include "_public.h"
#include "_mysql.h"
struct st_arg
{
  char connstr[101];     // 数据库的连接参数。// string for connection to the database.
  char charset[51];      // 数据库的字符集。// database character set / utf-8?
  char selectsql[1024];  // 从数据源数据库抽取数据的SQL语句。// SQL statement to extract data from the data source database.
  char fieldstr[501];    // 抽取数据的SQL语句输出结果集字段名，字段名之间用逗号分隔。 ... // SQL statement to extract data from the data source database.
  char fieldlen[501];    // 抽取数据的SQL语句输出结果集字段的长度，用逗号分隔。 // length of the field name of the result set output by the SQL statement to extract data from the data source database, separated by commas.
  char bfilename[31];    // 输出xml文件的前缀。// prefix of the output xml file.
  char efilename[31];    // 输出xml文件的后缀。// suffix of the output xml file.
  char outpath[301];     // 输出xml文件存放的目录。// directory where the output xml file is stored.
  int  maxcount;         // 输出xml文件最大记录数，0表示无限制。// maximum number of records in the output xml file, 0 means no limit.
  char starttime[52];    // 程序运行的时间区间 /// time interval for program operation
  char incfield[31];     // 递增字段名。// incremental field name.
  char incfilename[301]; // 已抽取数据的递增字段最大值存放的文件。// file name where the maximum value of the incremental field of the extracted data is stored.
  char connstr1[101];    // 已抽取数据的递增字段最大值存放的数据库的连接参数。// connection parameters of the database where the maximum value of the incremental field of the extracted data is stored.
  int  timeout;          // 进程心跳的超时时间。 // timeout of process heartbeat.
  char pname[51];        // 进程名，建议用"dminingmysql_后缀"的方式。// process name, it is recommended to use the suffix of "dminingmysql_".
} starg;

#define MAXFIELDCOUNT  100  // 结果集字段的最大数。 // maximum number of fields in the result set.
//#define MAXFIELDLEN    500  // 结果集字段值的最大长度。
int MAXFIELDLEN=-1;   // 结果集字段值的最大长度，存放fieldlen数组中元素的最大值。

char strfieldname[MAXFIELDCOUNT][31];    // 结果集字段名数组，从starg.fieldstr解析得到。//holds the field name of the result set, parsed from starg.fieldstr.
// the each max length of the fieldname is 32(0-31)
int  ifieldlen[MAXFIELDCOUNT];           // 结果集字段的长度数组，从starg.fieldlen解析得到。
int  ifieldcount;                        // strfieldname和ifieldlen数组中有效字段的个数。
int  incfieldpos=-1;                     // 递增字段在结果集数组中的位置。

connection conn,conn1;

CLogFile logfile;

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

void _help();

// 把xml解析到参数starg结构中。// parse xml to parameter starg structure.
bool _xmltoarg(char *strxmlbuffer);

// 判断当前时间是否在程序运行的时间区间内。// determine whether the current time is within the time interval of program operation.
bool instarttime();

// 数据抽取的主函数。// the main function of data extraction.
bool _dminingmysql();

CPActive PActive;  // 进程心跳。

char strxmlfilename[301]; // xml文件名。 // xml file name.
void crtxmlfilename();    // 生成xml文件名。// function to generate xml file name.

long imaxincvalue;     // 自增字段的最大值。
bool readincfield();   // 从数据库表中或starg.incfilename文件中获取已抽取数据的最大id。// get the maximum id of the extracted data from the database table or starg.incfilename file.
bool writeincfield();  // 把已抽取数据的最大id写入数据库表或starg.incfilename文件。// write the maximum id of the extracted data to the database table or starg.incfilename file.

int main(int argc,char *argv[])
{
  if (argc!=3) { _help(); return -1; }

  // 关闭全部的信号和输入输出。
  // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
  // 但请不要用 "kill -9 +进程号" 强行终止。
  CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  // 打开日志文件。
  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("fail to open the logfile(%s)。\n",argv[1]); return -1;
  }

  // 解析xml，得到程序运行的参数。
  if (_xmltoarg(argv[2])==false) return -1;

  // 判断当前时间是否在程序运行的时间区间内。
  if (instarttime()==false) return 0;

  PActive.AddPInfo(starg.timeout,starg.pname);  // 把进程的心跳信息写入共享内存。
  // 注意，在调试程序的时候，可以启用类似以下的代码，防止超时。
  // PActive.AddPInfo(5000,starg.pname);

  // 连接数据源的数据库。
  if (conn.connecttodb(starg.connstr,starg.charset)!=0)
  {
    logfile.Write("connect database(%s) failed.\n%s\n",starg.connstr,conn.m_cda.message); return -1;
  }
  logfile.Write("connect database(%s) ok.\n",starg.connstr);

  // 连接本地的数据库，用于存放已抽取数据的自增字段的最大值。
  if (strlen(starg.connstr1)!=0)
  {
    if (conn1.connecttodb(starg.connstr1,starg.charset)!=0)
    {
      logfile.Write("connect database(%s) failed.\n%s\n",starg.connstr1,conn1.m_cda.message); return -1;
    }
    logfile.Write("connect database(%s) ok.\n",starg.connstr1);
  }

  _dminingmysql();

  return 0;
}

// 数据抽取的主函数。
bool _dminingmysql()
{
  // 从数据库表中或starg.incfilename文件中获取已抽取数据的最大id。
  readincfield(); // get the maximum id of the extracted data from the database table or starg.incfilename file.

  sqlstatement stmt(&conn);
  stmt.prepare(starg.selectsql);
  char strfieldvalue[ifieldcount][MAXFIELDLEN+1];  // 抽取数据的SQL执行后，存放结果集字段值的数组。
  for (int ii=1;ii<=ifieldcount;ii++)
  {
    stmt.bindout(ii,strfieldvalue[ii-1],ifieldlen[ii-1]); //bindout is used to bind the result set field value to the array strfieldvalue.
    // after execute the bindout value will be stored in the array strfieldvalue.
  }

  // 如果是增量抽取，绑定输入参数（已抽取数据的最大id）。
  if (strlen(starg.incfield)!=0) stmt.bindin(1,&imaxincvalue);

  if (stmt.execute()!=0)
  {
    logfile.Write("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message); return false;
  }

  PActive.UptATime();

  CFile File;   // 用于操作xml文件。

  while (true)
  {
    memset(strfieldvalue,0,sizeof(strfieldvalue));

    if (stmt.next()!=0) break;

    if (File.IsOpened()==false)
    {
      crtxmlfilename();   // 生成xml文件名。

      if (File.OpenForRename(strxmlfilename,"w+")==false)
      {
        logfile.Write("File.OpenForRename(%s) failed.\n",strxmlfilename); return false;
      }

      File.Fprintf("<data>\n");
    }

    for (int ii=1;ii<=ifieldcount;ii++)
    {
      printf("<%s>%s</%s>",strfieldname[ii-1],strfieldvalue[ii-1],strfieldname[ii-1]);
      File.Fprintf("<%s>%s</%s>",strfieldname[ii-1],strfieldvalue[ii-1],strfieldname[ii-1]);
    }
    File.Fprintf("<endl/>\n");

    // 如果记录数达到starg.maxcount行就切换一个xml文件。
    if ( (starg.maxcount>0) && (stmt.m_cda.rpc%starg.maxcount==0) )
    {
      File.Fprintf("</data>\n");

      if (File.CloseAndRename()==false)
      {
        logfile.Write("File.CloseAndRename(%s) failed.\n",strxmlfilename); return false;
      }

      logfile.Write("生成文件%s(%d)。\n",strxmlfilename,starg.maxcount);      

      PActive.UptATime();
    }

    // 更新自增字段的最大值。
    if ( (strlen(starg.incfield)!=0) && (imaxincvalue<atol(strfieldvalue[incfieldpos])) )
       imaxincvalue=atol(strfieldvalue[incfieldpos]);
  }

  if (File.IsOpened()==true)
  {
    File.Fprintf("</data>\n");

    if (File.CloseAndRename()==false)
    {
      logfile.Write("File.CloseAndRename(%s) failed.\n",strxmlfilename); return false;
    }

    if (starg.maxcount==0)
      logfile.Write("生成文件%s(%d)。\n",strxmlfilename,stmt.m_cda.rpc);
    else
      logfile.Write("生成文件%s(%d)。\n",strxmlfilename,stmt.m_cda.rpc%starg.maxcount);
  }

  // 把已抽取数据的最大id写入数据库表或starg.incfilename文件。
  if (stmt.m_cda.rpc>0) writeincfield();

  return true;
}

void EXIT(int sig)
{
  printf("程序退出，sig=%d\n\n",sig);

  exit(0);
}

void _help()
{
  printf("Using:/project/tools1/bin/dminingmysql logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools/bin/procctl 3600 /home/cis623-vm/root/project/tools/bin/dminingmysql /home/cis623-vm/root/log/idc/dminingmysql_ZHOBTCODE.log \"<connstr>127.0.0.1,root,cis623,mysql,3306</connstr><charset>utf8</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/home/cis623-vm/root/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysql_ZHOBTCODE</pname>\"\n\n");
  printf("       /project/tools/bin/procctl   30 /home/cis623-vm/root/project/tools/bin/dminingmysql /home/cis623-vm/root/log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>127.0.0.1,root,cis623,mysql,3306</connstr><charset>utf8</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid > 1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/home/cis623-vm/root/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/home/cis623-vm/root/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout><pname>dminingmysql_ZHOBTMIND_HYCZ</pname><maxcount>1000</maxcount><connstr1>127.0.0.1,root,cis623,mysql,3306</connstr1>\"\n\n");
  
  printf("本程序是数据中心的公共功能模块，用于从MySQL数据库源表抽取数据，生成xml文件。\n");
  printf("logfilename 本程序运行的日志文件。\n");
  printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

  printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
  printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
  printf("selectsql   从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩一个。\n");
  printf("fieldstr    抽取数据的SQL语句输出结果集字段名，中间用逗号分隔，将作为xml文件的字段名。\n");
  printf("fieldlen    抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应。\n");
  printf("bfilename   输出xml文件的前缀。\n");
  printf("efilename   输出xml文件的后缀。\n");
  printf("outpath     输出xml文件存放的目录。\n");
  printf("maxcount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
  printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
         "如果starttime为空，那么starttime参数将失效，只要本程序启动就会执行数据抽取，为了减少数据源"\
         "的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候时进行。\n");
  printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
          "如果incfield为空，表示不采用增量抽取方案。");
  printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
  printf("connstr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先。");
  printf("timeout     本程序的超时时间，单位：秒。\n");
  printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
  printf("This program is a public function module of the data center, used to extract data from MySQL database source tables and generate XML files.\n");
printf("logfilename This is the log file of the program's execution.\n");
printf("xmlbuffer This is the parameter of the program's execution, represented in XML, specifically as follows:\n\n");

printf("connstr Connection parameters for the database, in the format: ip, username, password, dbname, port.\n");
printf("charset The character set of the database. This parameter should be consistent with the source database, otherwise there will be issues with Chinese characters being garbled.\n");
printf("selectsql The SQL statement to extract data from the source database. Note: The percentage sign  needs four to display two after being prepared.\n");
printf("fieldstr Field names of the result set of the SQL statement extracting data, separated by commas, will be used as the field names of the XML file.\n");
printf("fieldlen Length of the fields in the result set of the SQL statement extracting data, separated by commas. The fields in fieldstr and fieldlen must correspond one-to-one.\n");
printf("bfilename The prefix of the output XML file.\n");
printf("efilename The suffix of the output XML file.\n");
printf("outpath The directory where the output XML file is stored.\n");
printf("maxcount The maximum number of records in the output XML file, the default is 0, which means no limit. If this parameter is set to 0, be sure to increase the value of timeout to prevent the program from timing out.\n");
printf("starttime The time interval of the program's execution, for example 02,13 means: If the program starts at 02 and 13 hours, it will run, otherwise it will not run."
"If starttime is empty, then the starttime parameter will be invalid, and the data extraction will be executed as long as the program starts. To reduce the pressure on the data source,"
"when extracting data from the database, it is generally done when the other party's database is the most idle.\n");
printf("incfield The incremental field name, it must be a field name in fieldstr, and it can only be an integer, generally an auto-increment field."
"If incfield is empty, it means that the incremental extraction plan is not adopted.");
printf("incfilename The file where the maximum value of the extracted data's incremental field is stored. If this file is lost, all data will be extracted again.\n");
printf("connstr1 The connection parameter of the database where the maximum value of the extracted data's incremental field is stored. Choose one of connstr1 and incfilename, with connstr1 being the priority.");
printf("timeout The timeout of the program, in seconds.\n");
printf("pname The process name, try to use an easy-to-understand name that is different from other processes to facilitate troubleshooting.\n\n\n");
}

// 把xml解析到参数starg结构中。
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"connstr",starg.connstr,100);
  if (strlen(starg.connstr)==0) { logfile.Write("connstr is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"charset",starg.charset,50);
  if (strlen(starg.charset)==0) { logfile.Write("charset is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"selectsql",starg.selectsql,1000);
  if (strlen(starg.selectsql)==0) { logfile.Write("selectsql is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"fieldstr",starg.fieldstr,500);
  if (strlen(starg.fieldstr)==0) { logfile.Write("fieldstr is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"fieldlen",starg.fieldlen,500);
  if (strlen(starg.fieldlen)==0) { logfile.Write("fieldlen is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"bfilename",starg.bfilename,30);
  if (strlen(starg.bfilename)==0) { logfile.Write("bfilename is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"efilename",starg.efilename,30);
  if (strlen(starg.efilename)==0) { logfile.Write("efilename is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"outpath",starg.outpath,300);
  if (strlen(starg.outpath)==0) { logfile.Write("outpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"starttime",starg.starttime,50);  // 可选参数。

  GetXMLBuffer(strxmlbuffer,"incfield",starg.incfield,30);  // 可选参数。

  GetXMLBuffer(strxmlbuffer,"incfilename",starg.incfilename,300);  // 可选参数。

  GetXMLBuffer(strxmlbuffer,"maxcount",&starg.maxcount);  // 可选参数。

  GetXMLBuffer(strxmlbuffer,"connstr1",starg.connstr1,100);  // 可选参数。

  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);   // 进程心跳的超时时间。
  if (starg.timeout==0) { logfile.Write("timeout is null.\n");  return false; }

  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);     // 进程名。
  if (strlen(starg.pname)==0) { logfile.Write("pname is null.\n");  return false; }

  // 1、把starg.fieldlen解析到ifieldlen数组中；
  CCmdStr CmdStr;

  CmdStr.SplitToCmd(starg.fieldlen,",");

  // 判断字段数是否超出MAXFIELDCOUNT的限制。
  if (CmdStr.CmdCount()>MAXFIELDCOUNT)
  {
    //logfile.Write("fieldlen的字段数太多，超出了最大限制%d。\n",MAXFIELDCOUNT); return false;
    logfile.Write("The number of fields in fieldlen is too many, exceeding the maximum limit %d.\n", MAXFIELDCOUNT); return false;
  }

  for (int ii=0;ii<CmdStr.CmdCount();ii++)
  {
    CmdStr.GetValue(ii,&ifieldlen[ii]);
    // if (ifieldlen[ii]>MAXFIELDLEN) ifieldlen[ii]=MAXFIELDLEN;   // 字段的长度不能超过MAXFIELDLEN。
    if (ifieldlen[ii]>MAXFIELDLEN) MAXFIELDLEN=ifieldlen[ii];   // 得到字段长度的最大值。
  }

  ifieldcount=CmdStr.CmdCount();

  // 2、把starg.fieldstr解析到strfieldname数组中；
  CmdStr.SplitToCmd(starg.fieldstr,",");

  // 判断字段数是否超出MAXFIELDCOUNT的限制。
  if (CmdStr.CmdCount()>MAXFIELDCOUNT)
  {
    //logfile.Write("fieldstr的字段数太多，超出了最大限制%d。\n",MAXFIELDCOUNT); return false;
    logfile.Write("The number of fields in fieldstr is too many, exceeding the maximum limit %d.\n", MAXFIELDCOUNT); return false;
  }

  for (int ii=0;ii<CmdStr.CmdCount();ii++)
  {
    CmdStr.GetValue(ii,strfieldname[ii],30);
  }

  // 判断strfieldname和ifieldlen两个数组中的字段是否一致。
  if (ifieldcount!=CmdStr.CmdCount())
  {
    //logfile.Write("fieldstr和fieldlen的元素数量不一致。\n"); return false;
    logfile.Write("The number of elements in fieldstr and fieldlen is not consistent.\n"); return false;
  }

  // 3、获取自增字段在结果集中的位置。
  // find the self-inc field (index) position in the result set.
  if (strlen(starg.incfield)!=0)
  {
    for (int ii=0;ii<ifieldcount;ii++)
      if (strcmp(starg.incfield,strfieldname[ii])==0) { incfieldpos=ii; break; }

    if (incfieldpos==-1)
    {
      //logfile.Write("递增字段名%s不在列表%s中。\n",starg.incfield,starg.fieldstr); return false;
       logfile.Write("Increment field name %s is not in the list %s.\n", starg.incfield, starg.fieldstr); return false;
    }

    if ( (strlen(starg.incfilename)==0) && (strlen(starg.connstr1)==0) )
    {
      //logfile.Write("incfilename和connstr1参数必须二选一。\n"); return false;
      logfile.Write("One of the incfilename and connstr1 parameters must be chosen.\n"); return false;
    }
  }

  return true;
}

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime()
{
  // 程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。
  if (strlen(starg.starttime)!=0)
  {
    char strHH24[3];
    memset(strHH24,0,sizeof(strHH24));
    LocalTime(strHH24,"hh24");  // 只获取当前时间中的小时。
    if (strstr(starg.starttime,strHH24)==0) return false;
  }

  return true;
}

void crtxmlfilename()   // 生成xml文件名。// Generate xml file name.
{
  // xml全路径文件名=start.outpath+starg.bfilename+当前时间+starg.efilename+序号.xml
  // xml full path file name = start.outpath + starg.bfilename + current time + starg.efilename + serial number.xml
  char strLocalTime[21];
  memset(strLocalTime,0,sizeof(strLocalTime));
  LocalTime(strLocalTime,"yyyymmddhh24miss");

  static int iseq=1;
  SNPRINTF(strxmlfilename,300,sizeof(strxmlfilename),"%s/%s_%s_%s_%d.xml",starg.outpath,starg.bfilename,strLocalTime,starg.efilename,iseq++);
}

// 从数据库表中或starg.incfilename文件中获取已抽取数据的最大id。
// Get the maximum id of the extracted data from the database table or the starg.incfilename file.
bool readincfield()   
{
  imaxincvalue=0;    // 自增字段的最大值。

  // 如果starg.incfield参数为空，表示不是增量抽取。
  if (strlen(starg.incfield)==0) return true;

  if (strlen(starg.connstr1)!=0) //connstr1 meaning we save the data in the database.
  {
    // 从数据库表中加载自增字段的最大值。
    // create table T_MAXINCVALUE(pname varchar(50),maxincvalue numeric(15),primary key(pname));
    sqlstatement stmt(&conn1);
    stmt.prepare("select maxincvalue from T_MAXINCVALUE where pname=:1");
    stmt.bindin(1,starg.pname,50);
    stmt.bindout(1,&imaxincvalue);//
    stmt.execute();
    stmt.next();
  }
  else
  {
    // 从文件中加载自增字段的最大值。
    CFile File;

    // 如果打开starg.incfilename文件失败，表示是第一次运行程序，也不必返回失败。// If the starg.incfilename file fails to open, it means that the program is running for the first time, and there is no need to return failure.
    // 也可能是文件丢了，那也没办法，只能重新抽取。// It may also be that the file is lost, then there is no way, can only re-extract.
    if (File.Open(starg.incfilename,"r")==false) return true;

    // 从文件中读取已抽取数据的最大id。
    char strtemp[31];
    File.FFGETS(strtemp,30);
    imaxincvalue=atol(strtemp);
  }

  //logfile.Write("上次已抽取数据的位置（%s=%ld）。\n",starg.incfield,imaxincvalue);
  logfile.Write("The position of the last extracted data (%s=%ld).\n", starg.incfield, imaxincvalue);

  return true;
}

// 把已抽取数据的最大id写入starg.incfilename文件。
bool writeincfield()  
{
  // 如果starg.incfield参数为空，表示不是增量抽取。
  if (strlen(starg.incfield)==0) return true;

  if (strlen(starg.connstr1)!=0)
  {
    // 把自增字段的最大值写入数据库的表。
    // create table T_MAXINCVALUE(pname varchar(50),maxincvalue numeric(15),primary key(pname));
    sqlstatement stmt(&conn1);
    stmt.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
    if (stmt.m_cda.rc==1146)
    {
      // 如果表不存在，就创建表。
      conn1.execute("create table T_MAXINCVALUE(pname varchar(50),maxincvalue numeric(15),primary key(pname))");
      conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
      conn1.commit();
      return true;
    }
    stmt.bindin(1,&imaxincvalue);
    stmt.bindin(2,starg.pname,50);
    if (stmt.execute()!=0)
    {
      logfile.Write("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message); return false;
    }
    if (stmt.m_cda.rpc==0)
    {
      conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
    }
    conn1.commit();
  }
  else
  {
    // 把自增字段的最大值写入文件。
    CFile File;

    if (File.Open(starg.incfilename,"w+")==false) 
    {
      logfile.Write("File.Open(%s) failed.\n",starg.incfilename); return false;
    }

    // 把已抽取数据的最大id写入文件。
    File.Fprintf("%ld",imaxincvalue);

    File.Close();
  }

  return true;
}

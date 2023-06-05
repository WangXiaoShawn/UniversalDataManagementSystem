/*
 * Program Name: idcapp.h The header file of idcapp.cpp.
 * Author: WangXiao
 * Email: WANGXIAOJOBHUNTING @GMAIL.COM
 * Date: 2023/6/4
*/

#ifndef IDCAPP_H
#define IDCAPP_H

#include "_public.h"
#include "_mysql.h"

struct st_zhobtmind
{
  char obtid[11];      // 站点代码。// Station number
  char ddatetime[21];  // 数据时间，精确到分钟。// Data time, accurate to minutes
  char t[11];          // 温度，单位：0.1摄氏度。// Temperature, unit: 0.1 degrees Celsius
  char p[11];          // 气压，单位：0.1百帕。// Air pressure, unit: 0.1 hPa
  char u[11];          // 相对湿度，0-100之间的值。// Relative humidity, a value between 0 and 100
  char wd[11];         // 风向，0-360之间的值。// Wind direction, a value between 0 and 360
  char wf[11];         // 风速：单位0.1m/s。// Wind speed: unit 0.1m/s
  char r[11];          // 降雨量：0.1mm。// Rainfall: 0.1mm
  char vis[11];        // 能见度：0.1米。// Visibility: 0.1 meters
};

// National station minute observation data operation class.
class CZHOBTMIND
{
public:
  connection  *m_conn;     // 数据库连接。 // Database connection.
  CLogFile    *m_logfile;  // 日志。// Create a log file.
  sqlstatement m_stmt;     // 插入表操作的sql。// Insert the sql of the table operation.
  char m_buffer[1024];   // 从文件中读到的一行。// Read a line from the file.
  struct st_zhobtmind m_zhobtmind; // 全国站点分钟观测数据结构。// National station minute observation data structure.

  CZHOBTMIND();
  CZHOBTMIND(connection *conn,CLogFile *logfile);

 ~CZHOBTMIND();

  void BindConnLog(connection *conn,CLogFile *logfile);  // 把connection和CLogFile的传进去。// Pass in connection and CLogFile.
  bool SplitBuffer(char *strBuffer,bool bisxml);  // 把从文件读到的一行数据拆分到m_zhobtmind结构体中。// Split a line of data read from the file into the m_zhobtmind structure.
  bool InsertTable();  // 把m_zhobtmind结构体中的数据插入到T_ZHOBTMIND表中。
};



#endif

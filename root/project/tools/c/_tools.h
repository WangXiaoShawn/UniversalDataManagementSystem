#ifndef _TOOLS_H
#define _TOOLS_H

#include "_public.h"
#include "_mysql.h"

// 表的列(字段)信息的结构体。//the structure of the column information of the table
struct st_columns
{
  char  colname[31];  // 列名。//columen name
  char  datatype[31]; // 列的数据类型，分为number、date和char三大类。 //data type, we use number ,data,char three types
  int   collen;       // 列的长度，number固定20，date固定19，char的长度由表结构决定。//the length of the column
  int   pkseq;        // 如果列是主键的字段，存放主键字段的顺序，从1开始，不是主键取值0。// if the column is the primary key, the order of the primary key field is stored, starting from 1, and the value is 0 if it is not the primary key.
};

// 获取表全部的列和主键列信息的类。//the class that gets all the columns and primary key column information of the table
class CTABCOLS
{
public:
  CTABCOLS();

  int m_allcount;   // 全部字段的个数。//the number of all columns
  int m_pkcount;    // 主键字段的个数。//the number of primary key columns
  int m_maxcollen;  //the maximum length of all columns

  vector<struct st_columns> m_vallcols;  // 存放全部字段信息的容器。//the container that stores all column information
  vector<struct st_columns> m_vpkcols;   // 存放主键字段信息的容器。//the container that stores primary key column information

  char m_allcols[3001];  // 全部的字段名列表，以字符串存放，中间用半角的逗号分隔。//the list of all columns, stored as a string, separated by a half-width comma.
  char m_pkcols[301];    // 主键字段名列表，以字符串存放，中间用半角的逗号分隔。//the list of primary key columns, stored as a string, separated by a half-width comma.

  void initdata();  // 成员变量初始化。

  // 获取指定表的全部字段信息。//get all the column information of the specified table
  bool allcols(connection *conn,char *tablename);

  // 获取指定表的主键字段信息。//get the primary key column information of the specified table
  bool pkcols(connection *conn,char *tablename);
};


#endif

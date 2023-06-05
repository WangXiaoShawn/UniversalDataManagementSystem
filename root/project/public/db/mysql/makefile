# mysql头文件存放的目录。
MYSQLINCL = -I/usr/local/mysql/include

# mysql库文件存放的目录。
MYSQLLIB = -L/usr/local/mysql/lib

# 需要链接的mysql库。# static link libmysqlclient.a
MYSQLLIBS = -lmysqlclient

CFLAGS = -g -Wno-write-strings

all: createtable inserttable updatetable selecttable

createtable: createtable.cpp _mysql.h _mysql.cpp
	g++ $(CFLAGS) -o createtable $(MYSQLINCL) $(MYSQLLIB) createtable.cpp _mysql.cpp $(MYSQLLIBS) -lm -lc

inserttable:inserttable.cpp _mysql.h _mysql.cpp
	g++ $(CFLAGS) -o inserttable $(MYSQLINCL) $(MYSQLLIB) inserttable.cpp _mysql.cpp $(MYSQLLIBS)  -lm -lc

updatetable:updatetable.cpp _mysql.h _mysql.cpp
	g++ $(CFLAGS) -o updatetable  $(MYSQLINCL) $(MYSQLLIB) updatetable.cpp _mysql.cpp $(MYSQLLIBS) -lm -lc

selecttable:selecttable.cpp _mysql.h _mysql.cpp
	g++ $(CFLAGS) -o selecttable $(MYSQLINCL) $(MYSQLLIB) selecttable.cpp _mysql.cpp $(MYSQLLIBS)  -lm -lc



clean:
	rm -rf createtable inserttable updatetable selecttable
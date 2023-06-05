# 开发框架头文件路径。
PUBINCL = -I/home/cis623-vm/root/project/public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
PUBCPP = /home/cis623-vm/root/project/public/_public.cpp

# mysql头文件存放路径
MYSQLINCL = -I/usr/local/mysql/include	-I/home/cis623-vm/root/project/public/db/mysql
#mysql库文件保存路径
MYSQLLIB = -L/usr/local/mysql/lib
# mysql链接库 
MYSQLLIBS = -lmysqlclient
# 开发框架mysql的cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。                                                                                                                             
MYSQLCPP = /home/cis623-vm/root/project/public/db/mysql/_mysql.cpp

CFLAGS = -g

all: crtsurfdata obtcodetodb obtmindtodb

crtsurfdata: crtsurfdata.cpp
	g++ $(CFLAGS) -o crtsurfdata crtsurfdata.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp crtsurfdata ../bin/.

obtcodetodb: obtcodetodb.cpp
	g++ $(CFLAGS) -o obtcodetodb obtcodetodb.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS)  -lm -lc
	cp obtcodetodb ../bin/.

obtmindtodb: obtmindtodb.cpp idcapp.h idcapp.cpp
	g++ $(CFLAGS) -o obtmindtodb obtmindtodb.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) idcapp.cpp  -lm -lc
	cp obtmindtodb ../bin/.

clean:
	rm -f crtsurfdata obtcodetodb obtmindtodb



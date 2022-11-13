.PHONY:all
all:cgi mysql_cgi compile_run.json

cgi:cgi.cc
	g++ -o $@ $^ -std=c++11
mysql_cgi:mysql_cgi.cc
#g++ -o $@ $^ -std=c++11 -I include -L lib -lmysqlclient -lpthread -ldl -static
	g++ -o $@ $^ -std=c++11 -I/usr/include/mysql -L/usr/lib64/mysql -lmysqlclient
compile_run.json:compile_run.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp -lpthread
.PHONY:clean
clean:
	rm -f mysql_cgi cgi compile_run.json

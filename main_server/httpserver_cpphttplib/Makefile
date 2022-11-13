bin=httpserver
src=main_server.cc
cc=g++
LD_FLAGS = -std=c++11 -lpthread #-DDEBUG=1
outputfile=../output

test_outputfile = $(shell if [ -d $(outputfile) ]; then echo "$(outputfile) file exist"; else echo "$(outputfile) file noexist"; fi)
test_httpserver = $(shell if [ -e $(bin) ]; then echo "$(bin) bin exist"; else echo "$(bin) bin noexist"; fi)
curr=$(shell pwd)


.PHONY:ALL
ALL:$(bin) CGI

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS) -lctemplate -ljsoncpp -pthread
#CGI:
#	cd ../ROOT/cgi-bin;\
#		make;\
#		cd $(curr);

.PHONY:clean
clean:
	$(info $(test_httpserver))
ifeq ("$(test_httpserver)", "$(bin) bin exist")#ifeq不能缩进
	rm -f $(bin)
endif
	
	$(info $(test_outputfile))
ifeq ("$(test_outputfile)", "$(outputfile) file exist")
	rm -rf $(outputfile);
endif
		
#	cd ../ROOT/cgi-bin;\
#		make clean;\
#		cd $(curr);

.PHONY:output #全部打包到output内
output:
	mkdir -p ../output
	cp $(bin) ../output
	cp -rf ../ROOT ../output
#	rm -f ../output/ROOT/cgi-bin/comm.hpp
#	rm -f ../output/ROOT/cgi-bin/Makefile
#	rm -f ../output/ROOT/cgi-bin/cgi.cc
#	rm -f ../output/ROOT/cgi-bin/mysql_cgi.cc
#	rm -f ../output/ROOT/cgi-bin/question_all.cc
#	rm -f ../output/ROOT/cgi-bin/question.cc
#	rm -f ../output/ROOT/cgi-bin/judge.cc
#	cp -rf ../ctemplatehtml ../output/ROOT/
#	mkdir -p ../output/ROOT/cgi-bin/
#	cp ../cgi/cgi ../output/ROOT/cgi-bin/
#	cp ../cgi/mysql_cgi ../output/ROOT/cgi-bin/
#	cp ../cgi/question_all ../output/ROOT/cgi-bin/
#	cp ../cgi/compile_run.json ../output/ROOT
#cp ./cgi/mysql_cgi output/ROOT


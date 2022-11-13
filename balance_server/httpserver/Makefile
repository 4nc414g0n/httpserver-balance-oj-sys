bin=httpserver
src=main.cc
cc=g++
LD_FLAGS = -std=c++11 -lpthread #-DDEBUG=1
outputfile=../output

test_outputfile = $(shell if [ -d $(outputfile) ]; then echo "$(outputfile) file exist"; else echo "$(outputfile) file noexist"; fi)
test_httpserver = $(shell if [ -e $(bin) ]; then echo "$(bin) bin exist"; else echo "$(bin) bin noexist"; fi)
curr=$(shell pwd)


.PHONY:ALL
ALL:$(bin) CGI

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)
CGI:
	cd $(curr)/../cgi;\
		make;\
		cd $(curr);

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
		
	cd ../cgi;\
		make clean;\
		cd $(curr);

.PHONY:output #全部打包到output内
output:
	mkdir -p ../output
	cp $(bin) ../output
	cp -rf ../ROOT ../output
	cp ../cgi/cgi ../output/ROOT
	cp ../cgi/mysql_cgi ../output/ROOT
	cp ../cgi/compile_run.json ../output/ROOT
#cp ./cgi/mysql_cgi output/ROOT


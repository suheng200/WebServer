.PHONY:clean
server:obj/web_server.o obj/thread_pool.o
	gcc -o ./bin/server src/main.c obj/web_server.o obj/thread_pool.o  -lpthread

obj/web_server.o:src/web_server.c
	gcc -c -o obj/web_server.o  src/web_server.c 


obj/thread_pool.o:src/thread_pool.c
	gcc -c -o obj/thread_pool.o  src/thread_pool.c
	
clean:
	-rm -rf ./obj/* ./bin/server

all:
	gcc client.c -o client
	gcc server.c -o server

pthreads:
	gcc client.c -o client
	gcc server.c -o server -DTHREAD_HANDLER -lpthread

clean:
	rm -f ./client ./server

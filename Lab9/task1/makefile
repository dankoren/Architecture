all: client server

client: client.o line_parser.h common.c
	gcc -g -Wall -o client client.o

client.o: client.c 
	gcc -g -Wall -c -o client.o client.c

server: server.o line_parser.h common.c
	gcc -g -Wall -o server server.o

server.o: server.c 
	gcc -g -Wall -c -o server.o server.c


clean: 
	rm -f *.o client server
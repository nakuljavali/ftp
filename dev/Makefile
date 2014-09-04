all: client.c server.c
	gcc -o client client.c
	gcc -o server server.c

client:
	rm -rf *.o client
	gcc -o client client.c

server:
	rm -rf *.o server
	gcc -o server server.c

clean: 
	rm -rf *.o client client.c~ server server.c~

all: chat client

chat: chat.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o
	gcc chat.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o chat

client: client.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o
	gcc client.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o client

chat.o: chat.c
	gcc -c chat.c

client.o: client.c
	gcc -c client.c

linemode.o: linemode.c
	gcc -c linemode.c

zipmode.o: zipmode.c
	gcc -c zipmode.c

my_socket.o: my_socket.c
	gcc -c my_socket.c

sockbuf.o: sockbuf.c
	gcc -c sockbuf.c

sslmode.o: sslmode.c
	gcc -c sslmode.c

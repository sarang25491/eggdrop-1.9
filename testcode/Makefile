all: chat client ipv6 sclient

chat: chat.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o
	gcc chat.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o chat -g

client: client.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o
	gcc client.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o client -g

sclient: sclient.o socks5.o socks4.o http.o base64.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o
	gcc sclient.o socks5.o socks4.o http.o base64.o linemode.o zipmode.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o sclient -g

ipv6: ipv6.o my_socket.o sockbuf.o linemode.o
	gcc ipv6.o my_socket.o sockbuf.o linemode.o -o ipv6 -g

ipv6.o: ipv6.c
	gcc -c ipv6.c -g

chat.o: chat.c
	gcc -c chat.c -g

client.o: client.c
	gcc -c client.c -g

linemode.o: linemode.c
	gcc -c linemode.c -g

zipmode.o: zipmode.c
	gcc -c zipmode.c -g

my_socket.o: my_socket.c
	gcc -c my_socket.c -g

sockbuf.o: sockbuf.c
	gcc -c sockbuf.c -g

sslmode.o: sslmode.c
	gcc -c sslmode.c -g

sclient.o: sclient.c
	gcc -c sclient.c -g

base64.o: base64.c
	gcc -c base64.c -g

http.o: http.c
	gcc -c http.c -g

socks4.o: socks4.c
	gcc -c socks4.c -g

socks5.o: socks5.c
	gcc -c socks5.c -g

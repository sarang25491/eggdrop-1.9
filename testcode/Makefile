CFLAGS=-g
PROGS=chat client sclient

all: $(PROGS)

chat: chat.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o
	gcc $(CFLAGS) chat.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o chat

client: client.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o
	gcc $(CFLAGS) client.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o client

sclient: sclient.o socks5.o socks4.o http.o base64.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o
	gcc $(CFLAGS) sclient.o socks5.o socks4.o http.o base64.o linemode.o zipmode.o my_poll.o my_socket.o sockbuf.o sslmode.o -lz -lssl -lcrypto -o sclient

clean:
	rm -f *.o $(PROGS)

# Now, isn't this much simpler? ;P
.c.o:
	gcc $(CFLAGS) -c $<

CC = gcc
CFLAGS = -Wall

# With default we want to build everything
all: PingClient server

PingClient: PingClient.c
	$(CC) $(CFLAGS) -o PingClient PingClient.c

server:
	python3 UDPPingerServer.py 12000 1

clean:
	rm -f PingClient server
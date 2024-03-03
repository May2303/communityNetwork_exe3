# Makefile for TCP project

all: tcp-receiver tcp-sender

TCP_Receiver: tcp-receiver.c
	gcc -o tcp-receiver tcp-receiver.c

TCP_Sender: tcp-sender.c
	gcc -o tcp-sender tcp-sender.c

clean:
	rm -f *.o tcp-receiver tcp-sender

runs:
	./tcp-receiver

runc:
	./tcp-sender

runs-strace:
	strace -f ./tcp-receiver

runc-strace:
	strace -f ./tcp-sender

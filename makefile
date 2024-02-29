# Makefile for TCP project

all: tcp-reciver tcp-sender

tcp-reciver: tcp-reciver.c
	gcc -o tcp-reciver tcp-reciver.c

tcp-sender: tcp-sender.c
	gcc -o tcp-sender tcp-sender.c

clean:
	rm -f *.o tcp-reciver tcp-sender

runs:
	./tcp-reciver

runc:
	./tcp-sender

runs-strace:
	strace -f ./tcp-reciver

runc-strace:
	strace -f ./tcp-sender

#Makefile for TCP Project
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

all: TCP_Receiver TCP_Sender

TCP_Receiver: tcp-receiver.c
	$(CC) $(CFLAGS) -o TCP_Receiver tcp-receiver.c

TCP_Sender: tcp-sender.c
	$(CC) $(CFLAGS) -o TCP_Sender tcp-sender.c

clean:
	rm -f *.o TCP_Receiver TCP_Sender random_file.bin received_file.bin

runs:
	./TCP_Receiver

runc:
	./TCP_Sender

runs-cubic:
	./TCP_Receiver -p 12345 -algo cubic

runs-reno:
	./TCP_Receiver -p 12345 -algo reno

runc-cubic:
	./TCP_Sender -ip localhost -p 12345 -algo cubic

runc-reno:
	./TCP_Sender -ip localhost -p 12345 -algo reno
runs-strace:
	strace -f ./TCP_Receiver

runc-strace:
	strace -f ./TCP_Sender

#Makefile for rudp Project
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

all: RUDP_Receiver RUDP_Sender

RUDP_Receiver: rudp_receiver.o rudp_api.o
	$(CC) $(CFLAGS) -o RUDP_Receiver rudp_receiver.o rudp_api.o

RUDP_Sender: rudp_sender.o rudp_api.o
	$(CC) $(CFLAGS) -o RUDP_Sender rudp_sender.o rudp_api.o

rudp_receiver.o: rudp_receiver.c rudp_api.h
	$(CC) $(CFLAGS) -c rudp_receiver.c

rudp_sender.o: rudp_sender.c rudp_api.h
	$(CC) $(CFLAGS) -c rudp_sender.c

rudp_api.o: rudp_api.c rudp_api.h
	$(CC) $(CFLAGS) -c rudp_api.c

clean:
	rm -f *.o RUDP_Receiver RUDP_Sender random_file.bin received_file.bin

runs:
	./RUDP_Receiver

runc:
	./RUDP_Sender

runs-strace:
	strace -f ./RUDP_Receiver

testc:
	./RUDP_Sender -ip localhost -p 12345

tests:
	./RUDP_Receiver -p 12345
runc-strace:
	strace -f ./RUDP_Sender


# Makefile for TCP project

all: TCP_Receiver TCP_Sender

TCP_Receiver: tcp-receiver.c
	gcc -o TCP_Receiver tcp-receiver.c

TCP_Sender: tcp-sender.c
	gcc -o TCP_Sender tcp-sender.c

clean:
	rm -f *.o TCP_Receiver TCP_Sender

runs:
	./TCP_Receiver -p 12345 -algo cubic

runc:
	./TCP_Sender -ip localhost -p 12345 -algo cubic

runs-strace:
	strace -f ./TCP_Receiver

runc-strace:
	strace -f ./TCP_Sender

# Makefile for UDP project

all: rdup_receiver rdup_sender

rdup_receiver: rdup_receiver.c
	gcc -o rdup_receiver rdup_receiver.c

rdup_sender: rdup_sender.c
	gcc -o rdup_sender rdup_sender.c

clean:
	rm -f *.o rdup_receiver rdup_sender

runs:
	./rdup_receiver

runc:
	./rdup_sender

runs-strace:
	strace -f ./rdup_receiver

runc-strace:
	strace -f ./rudp_client
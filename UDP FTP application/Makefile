all: sender receiver

sender: sender.o
	gcc -pthread -O3 -o sender sender.o

sender.o: sender.c sender.h
	gcc -O3 -c -Wall sender.c

receiver: receiver.o 
	gcc -pthread  -O3 -o receiver receiver.o 

receiver.o: receiver.c receiver.h
	gcc -O3 -c -Wall receiver.c

clean:
	rm -f receiver
	rm -f sender
	rm -f *.o 

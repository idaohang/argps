argps: argps.o
	g++ -O3 -Wall -g -lgps -o argps argps.o

argps.o: argps.c
	g++ -O3 -Wall -g -lgps -c argps.c

clean:
	rm -f argps argps *.o


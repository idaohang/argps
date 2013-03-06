argps: argps.o
	g++ -O3 -Wall -g -o argps argps.o

argps.o: argps.c
	g++ -O3 -Wall -g -c argps.c

clean:
	rm -f argps argps *.o


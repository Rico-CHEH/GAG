gag: server.o commands.o 
	gcc -Wall -g -o gag server.o commands.o  -lz

server.o: src/server.c
	gcc -Wall -g -c src/server.c

commands.o: src/commands.c
	gcc -Wall -g -c src/commands.c

clean:
	rm -f *.exe *.o

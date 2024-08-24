gag: 
	gcc -Wall -g -o gag src/server.c -lz
clean:
	rm -f *.exe *.o

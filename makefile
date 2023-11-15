server: main.c
		gcc -o server -g main.c

clean:
		rm -f server *.o

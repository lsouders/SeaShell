CC = gcc
CFLAGS = -g -std=c99 -Wall -fsanitize=address,undefined

mysh: mysh.o
	$(CC) $(CFLAGS) -o mysh mysh.o
mysh.o: mysh.c
	$(CC) $(CFLAGS) -c mysh.c

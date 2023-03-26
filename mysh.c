#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFSIZE 256
#define DELIMS 3

char delimiters[] = {'<', '>', '|'};

char buf[BUFFSIZE];  // buffer used for reading in input.
char** tokens;	     // array of char pointers to store tokens (last item is null).
int token_count;     // number of tokens stored in tokens.
char** read_arr;     // array to store strings read from input.
int read_arr_size;   // how many strings are in read_arr.
int tokens_capacity; // how many locations given to tokens by malloc.
int read_capacity;   // how many locations given to read_arr by malloc.

int test;

// free all allocated space given to read_arr.
void free_read_arr()
{
	if (read_arr == NULL) return; // nothing to do
	for (int i = 0; i < read_arr_size; i++) if (read_arr[i] != NULL) free(read_arr[i]);
	free(read_arr);
}

/**
 * Free all allocated space to the token array.
 */
void free_tokens()
{
	if (tokens == NULL) return; // nothing to do
	for (int i = 0; i < token_count; i++) if (tokens[i] != NULL) free(tokens[i]);
	free(tokens);
}

/**
 * Print out the un-tokenized input.
 */
void print_read_arr()
{
	for (int i = 0; i < read_arr_size; i++) {
		printf("%s\n", read_arr[i]);
	}
}

/**
 * Check if given character is one of the possible delimiters
 */
int is_delimiter(char c)
{
	for (int i = 0; i < DELIMS; i++) {
		if (c == delimiters[i]) return 0;
	}
	return 1;
}

/**
 * Print out all of the tokens.
 */
void print_tokens()
{
	for (int i = 0; i < token_count; i++) {
		printf("|%s|\n", tokens[i]);
	}
}

/**
 * Take input give from read_arr and split it into tokens.
 * (for now for testing purposes, just split into words (only delimiter is ' ')).
 */
void tokenize_input()
{
	int token_len, i, j;
	char tok[50];    // if any word is longer than 50 characters, we have bigger problems
	
	// set up before we start tokenizing
	tokens_capacity = 10;
	token_count = 0; // track the length of the current token we are adding.
	tokens = (char **) malloc(sizeof(char *) * 10); // start with 10 tokens, add more if necessary.

	// tokenize here
	for (i = 0; i < read_arr_size; i++) {
		token_len = 0;
		for (j = 0; j < BUFFSIZE; j++) {
			if (token_count + 2 >= tokens_capacity) {
				tokens_capacity += tokens_capacity;
				tokens = (char **) realloc(tokens, (sizeof(char *) * tokens_capacity));
			}
			if (read_arr[i][j] == '\n') {
				if (token_len == 0) break;
				tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
				memcpy(tokens[token_count], tok, token_len);
				tokens[token_count][token_len] = '\0';
				token_count++;
				break;
			}
			else if (read_arr[i][j] == ' ') {
				if (token_len == 0) continue; // nothingn to do
				tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
				if (tokens[token_count] == NULL) printf("malloc failure????\n");
				memcpy(tokens[token_count], tok, token_len);
				tokens[token_count][token_len] = '\0';
				token_count++;
				token_len = 0;
			}
			else if (is_delimiter(read_arr[i][j]) == 0) {
				if (token_len != 0) {
					tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
					memcpy(tokens[token_count], tok, token_len);
					tokens[token_count][token_len] = '\0';
					token_count++;
				}
				tokens[token_count] = (char *) malloc(sizeof(char) * 2);
				tokens[token_count][0] = read_arr[i][j];
				tokens[token_count][1] = '\0';
				token_count++;
				memset(tok, '\0', 50);
				token_len = 0;
			}
			else {
				tok[token_len] = read_arr[i][j];
				token_len++;
			}
		}
	}	
}

/**
 * Begin to parse input here. We will read until a newline character occurs.
 * Once we reach a newline, send it to read_arr for it to be tokenized
 */
int read_input(int fd)
{
	int bytes_read, return_val;
	// start off with 5 spots for strings of size 256 bytes (shouldn't need more)
	read_arr = (char**) malloc(sizeof(char *) * 5);
	
	read_capacity = 5;
	read_arr_size = 0;
	bytes_read = 0;
	while ( (return_val = read(fd, &(buf[bytes_read]), 1)) > 0) {
		if (return_val != 1) return 1; // read failed for some reason
		if (buf[bytes_read] == '\n') {
			read_arr[read_arr_size] = (char *) malloc(sizeof(char) * BUFFSIZE);
			if (read_arr[read_arr_size] == NULL) exit(EXIT_FAILURE);
			memcpy(read_arr[read_arr_size], buf, (bytes_read + 1));
			read_arr_size++;
			return 0;
		}
		if (bytes_read + 1 == BUFFSIZE) {
			if (read_arr_size == 5) {
				read_arr = realloc(read_arr, (sizeof(char *) * 5));
			}
			read_arr[read_arr_size] = (char *) malloc(sizeof(char) * BUFFSIZE);
			memcpy(read_arr[read_arr_size], buf, BUFFSIZE);
			bytes_read = 0;
			read_arr_size++;
			continue;
		}
		bytes_read++;
	}
	return 1; // should never reach this line, should have reached endline by now.
}

/** 
 * Main driver for running the shell.
 * Here we will call our read_input function, which will gather a line of command
 * from the given file descriptor. We will then need to tokenize the command,
 * and then decide what to do with the tokens.
 */
int start_shell(int fd)
{
	while (1) {
		write(STDOUT_FILENO, "mysh> ", 6);
		if (read_input(fd) == 1) {
			printf("something wrong happened\n");
			return 0;
		}
		tokenize_input();
		// do something with input
		print_tokens();

		// free read array (and eventually tokens array
		free_read_arr();
		free_tokens();
	}
}


int main(int argc, char **argv) {

	// try opening the file passed in through argument 1
	int fd = STDIN_FILENO;
	switch (argc) {
		case 2:
			fd = open(argv[1], O_RDONLY);
			if (fd < 0) {
				write(STDOUT_FILENO, "File does not exist or cannot be opened!\n", 41);
				return EXIT_FAILURE;
			}
			break;
		case 1: 
			write(STDOUT_FILENO, "Welcome to our shell! Use at your own risk.\n", 44);
			if (start_shell(fd) == 1) {
				write(STDOUT_FILENO, "Error\n", 6);
			}
			else {
				write(STDOUT_FILENO, "Good\n", 5);
			}
			break;
	}

	
	return EXIT_SUCCESS;

}

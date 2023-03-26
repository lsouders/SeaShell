#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#define DIRECTORIES 6
#define BUILTIN 3
#define BUFFSIZE 256
#define DELIMS 3

char delimiters[] = {'<', '>', '|'};
char* builtin[] = {"exit", "cd", "pwd"};
char* directories[] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin"}; 


char buf[BUFFSIZE];  // buffer used for reading in input.
char** tokens;	     // array of char pointers to store tokens (last item is null).
int token_count;     // number of tokens stored in tokens.
char** read_arr;     // array to store strings read from input.
int read_arr_size;   // how many strings are in read_arr.
int tokens_capacity; // how many locations given to tokens by malloc.
int read_capacity;   // how many locations given to read_arr by malloc.
int pipe_info[2];    // stores index of first command and second command for pipe.




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
	// assure last item is null terminated
	if (token_count == tokens_capacity) {
		char** temp = (char **) realloc(tokens, (token_count + 1));
		if (temp == NULL) exit(EXIT_FAILURE);
		tokens = temp;
		tokens[token_count] = NULL;
	}
	else {
		tokens[token_count] = NULL;
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
 * Builtin implementation of change directory.
 */
int mycd()
{
	if (tokens[1] == NULL) {
		write(STDOUT_FILENO, "cd: needs an arguement!\n", 24);
		return 1;
	}
	else {
		if (chdir(tokens[1]) != 0) {
			perror("cd");
			return 1;
		}
	}
	return 0;
}

/**
 * Builtin implementation of pwd (print working directory).
 */
int mypwd()
{
	char* dir;
	char buffer[BUFFSIZE];
	int i, bytes = 0;

	if ( (dir = getcwd(buffer, BUFFSIZE)) == NULL) {
		perror("pwd");
		return 1;
	}

	// count bytes in buffer.
	for (i = 0; i < BUFFSIZE; i++) {
		if (buffer[i] == '\0') break;
		bytes++;
	}

	// display current working directory.
	write(STDOUT_FILENO, buffer, bytes);
	write(STDOUT_FILENO, "\n", 1);
	return 0;
}

/**
 * Checks to see if this program is a build in program.
 * Will return:
 *     -1: not built in command.
 *	0: successful run of command.
 *	1: command failed.
 */
int handle_builtin()
{
	int i, command = 0;
	for (i = 0; i < BUILTIN; i++) {
		if (strcmp(tokens[0], builtin[i]) == 0) {
			command = i + 1;
			break;
		}
	}

	// call built in program if possible.
	switch(command) {
	case 1:	
		write(STDOUT_FILENO, "mysh: exiting\n", 14);
		exit(EXIT_SUCCESS);
	case 2:
		return mycd();
	case 3:
		return mypwd();
	default:
		break;
	}
	return -1; // not a built in command.
}

/**
 * Search set of directories for given program name.
 */
int find_command()
{
	int i, j, dir_count = 0, name_count = 0;
	char* str;
	struct stat *statbuf;
	statbuf = malloc(sizeof(struct stat));

	i = 0;
	while (tokens[0][i] != '\0') {
		i++;
		name_count++;
	}

	for (i = 0; i < DIRECTORIES; i++) {
		j = 0;
		dir_count = 0;
		while (directories[i][j] != '\0') {
			j++;
			dir_count++;
		}
		str = (char *) malloc(sizeof(char) * (dir_count + name_count + 1));
		memcpy(str, directories[i], dir_count);
		memcpy(&(str[dir_count+1]), tokens[0], name_count);
		str[dir_count + name_count] = '\0';
		if (stat(str, statbuf) == 0) {
			free(str);
			free(statbuf);
			return 0;
		}
		free(str);
	}
	free(statbuf);
	return 1;
}

/**
 * Start a "simple" process, meaning no pipes or redirects.
 */
int simple_process()
{
	int pid, id, wait_status;
	
	// if we cannot find/access the file, return non-zero exit status.	
	if (!find_command()) {
		return 1;
	}
	
	pid = fork();
	if (pid == 0) {
		// need to somehow determine arguments?
		if (execvp(tokens[0], tokens) < 0) {
			perror(tokens[0]);
		}
		return 1;
	}
	else if (pid < 0) {
		perror("fork");
	}
	else {
		id = wait(&wait_status);
		if (WIFEXITED(wait_status)) {
			return WEXITSTATUS(wait_status); // return exits status.
		} else {
			return 1; // process exited abnormally.
		}
	}
	return 1;
}

/**
 * Determine if there is a pipe, and store indecies of commands surrounding pipe.
 */
int has_pipe()
{
	int i;

	pipe_info[0] = 0;
	pipe_inf0[1] = -1;
	
	// search over tokens to find pipe.
	for (i = 0; i < token_count; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			pip_info[1] = i;
			return 0;
		}
	}
	return 1;
}

/**
 * Call a program containing a pipe.
 */
int call_program_with_pipe()
{

}

/**
 * We must determine what type of program(s) we are calling, as well as if there
 * is any piping or redirection occurring. I could be wrong in assuming this, but 
 * if redirection and a pipe are in the same command line, then it only makes sense
 * to have < before | and > after |. 
 *
 * Different types of ways we may need to call a program:
 *	- ./someProgram : some executable in some directory
 *	- echo foo	: a system function where we call exec
 * 	- cd someDir    : a function we built into the shell
 */
int call_program()
{
	int status = 0;

	// check first for pipes then handle calling them.
	if (has_pipe() == 0) {
		call_program_with_pipe();		
	}
	
	// check if there are any redirects
		

	// check if we have a build in command
	switch (handle_builtin()) {
	case 0:
		return 0;
	case 1:
		return 1;
	case -1:
		break;
	default:
		break;
	}

	// check if there are pipes/redirects, send off to be handled
	status = simple_process();

	return status;
}

/** 
 * Main driver for running the shell.
 * Here we will call our read_input function, which will gather a line of command
 * from the given file descriptor. We will then need to tokenize the command,
 * and then decide what to do with the tokens.
 */
int start_shell(int fd)
{
	int status = 0;
	while (1) {
		if (fd == STDIN_FILENO) {
			if (status == 0) write(STDOUT_FILENO, "mysh> ", 6);
			else 		 write(STDOUT_FILENO, "!mysh> ", 7);
		}
		if (read_input(fd) == 1) {
			// nothing left to read.
			return 0;
		}
		tokenize_input();
		// do something with input
		//print_tokens();
		status = call_program();

		// free read array (and eventually tokens array
		free_read_arr();
		free_tokens();
	}
}


int main(int argc, char **argv) {

	// try opening the file passed in through argument 1
	int fd = STDIN_FILENO;
	switch (argc) {
		case 1: 
			write(STDOUT_FILENO, "Welcome to our shell! Use at your own risk.\n", 44);
			start_shell(fd); 
			break;
		default:
			write(STDOUT_FILENO, "Welcome to our shell! Use at your own risk.\n", 44);
			int fd = open(argv[1], O_RDONLY);
			if (fd < 0) {
				perror("open");
				exit(EXIT_FAILURE);
			}
			start_shell(fd);
	}
	
	return EXIT_SUCCESS;

}

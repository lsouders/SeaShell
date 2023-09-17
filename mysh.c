#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>


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

// global variables for split_args... send to top of file later.
char** args1;
char** args2; 
char** args3;
char** args4;
int args_lists_count;
int args1_count, args2_count, args3_count, args4_count;

int expand_wildcard(char* str, int length);

/**
 * Free all argument lists.
 */
void free_args()
{
	if (args1 != NULL) free(args1);
	if (args2 != NULL) free(args2);
	if (args3 != NULL) free(args3);
	if (args4 != NULL) free(args4);
}

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
int tokenize_input()
{
	int wildcard_flag, token_len, i, j;
	char tok[50];    // if any word is longer than 50 characters, we have bigger problems
	
	// set up before we start tokenizing
	tokens_capacity = 10;
	token_count = 0; // track the length of the current token we are adding.
	tokens = (char **) malloc(sizeof(char *) * 10); // start with 10 tokens, add more if necessary.

	// tokenize here
	for (i = 0; i < read_arr_size; i++) {
		memset(tok, '\0', 50);
		token_len = 0, wildcard_flag = 0;
		for (j = 0; j < BUFFSIZE; j++) {
			if (token_count + 2 >= tokens_capacity) {
				tokens_capacity += tokens_capacity;
				tokens = (char **) realloc(tokens, (sizeof(char *) * tokens_capacity));
			}
			if (read_arr[i][j] == '*') wildcard_flag = 1;
			if (read_arr[i][j] == '\n') {
				if (token_len == 0) break;
				if (wildcard_flag) {
					if (!expand_wildcard(tok, token_len)) {
						//token_count++;
						return 1;
					}
					wildcard_flag = 0;
				} else {
					tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
					memcpy(tokens[token_count], tok, token_len);
					tokens[token_count][token_len] = '\0';
					token_count++;
					memset(tok, '\0', 50);
				}
				break;
			}
			else if (read_arr[i][j] == '\\') {
				if (read_arr[i][j+1] == '\n') continue; // end of input line, nothing to do
				tok[token_len] = read_arr[i][j+1];
				j++;
				token_len++;
			}
			else if (read_arr[i][j] == ' ') {
				if (token_len == 0) continue; // nothingn to do
				if (wildcard_flag) {
					if (!expand_wildcard(tok, token_len)) {
						//token_count++;
						return 1;
					}
					wildcard_flag = 0;
				} else {
					tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
					if (tokens[token_count] == NULL) printf("malloc failure????\n");
					memcpy(tokens[token_count], tok, token_len);
					tokens[token_count][token_len] = '\0';
					token_count++;
				}
				memset(tok, '\0', 50);
				token_len = 0;
			}
			else if (is_delimiter(read_arr[i][j]) == 0) {
				if (token_len != 0) {
					if (wildcard_flag) {
						if (!expand_wildcard(tok, token_len)) {
					//		token_count++;
							return 1;
						}
						wildcard_flag = 0;
					} else {
						tokens[token_count] = (char *) malloc(sizeof(char) * (token_len + 1));
						memcpy(tokens[token_count], tok, token_len);
						tokens[token_count][token_len] = '\0';
						token_count++;
					}
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
	return 0;
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
 * Helper function to reduce redundency of split_args().
 * Return number of tokens added to the argument list.
 */
int split_args_helper(char** args, int i, int counter)
{
	for (; i < token_count; i++) {
		if (is_delimiter(tokens[i][0]) != 0) {
			args[counter] = tokens[i];
			counter++;
		}
		else break;
	}

	return counter;
}

/**
 * Split the array of tokens into lists of input arguments.
 * We will NOT include redirection/pipes in args lists. Just program names and their arguments.
 */
void split_args()
{
	int i = 0;
	
	args1 = (char **) malloc(sizeof(char *) * (token_count + 1));
	args2 = (char **) malloc(sizeof(char *) * token_count);
	args3 = (char **) malloc(sizeof(char *) * token_count);
	args4 = (char **) malloc(sizeof(char *) * token_count);

	args1_count = split_args_helper(args1, i, 0);
	i = args1_count + 1;
	args1[args1_count] = NULL;
	if (i == token_count) return;
	args2_count = split_args_helper(args2, i, 0);
	i += args2_count + 1;
	if (i == token_count) return;
	args3_count = split_args_helper(args3, i, 0);
	i += args3_count + 1;
	if (i == token_count) return;
	args4_count = split_args_helper(args4, i, 0);
}

/**
 * Print out the args list.
 */
void print_args()
{
	int i;
	for (i = 0; i < args1_count; i++) printf("|%s| ", args1[i]);
	printf("args1: %d\n", args1_count);
	for (i = 0; i < args2_count; i++) printf("|%s| ", args2[i]);
	printf("args2: %d\n", args2_count);
	for (i = 0; i < args3_count; i++) printf("|%s| ", args3[i]);
	printf("args3: %d\n", args3_count);
	for (i = 0; i < args4_count; i++) printf("|%s| ", args4[i]);
	printf("args4: %d\n", args4_count);
}

/**
 * Builtin implementation of change directory.
 */
int mycd(char** args)
{
	if (args[1] == NULL) {
		// change to home directory
		char* home = getenv("HOME");
		if (chdir(home) != 0) {
			perror("cd");
			return 1;
		}
	}
	else {
		if (chdir(args[1]) != 0) {
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
int handle_builtin(char* prog, char** args)
{
	int i, command = 0;
	for (i = 0; i < BUILTIN; i++) {
		if (strcmp(prog, builtin[i]) == 0) {
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
		return mycd(args);
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
int find_command(int index)
{
	int i, j, dir_count = 0, name_count = 0;
	char* str;
	struct stat *statbuf;
	statbuf = malloc(sizeof(struct stat));

	i = 0;
	while (tokens[index][i] != '\0') {
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
		memcpy(&(str[dir_count]), tokens[index], name_count);
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
	if (find_command(0) == 1) {
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
	pipe_info[1] = -1;
	
	// search over tokens to find pipe.
	for (i = 0; i < token_count; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			pipe_info[1] = i+1;
			return 0;
		}
	}
	return 1;
}

/**
 * Check if piped command has redirection as well.
 */
int pipe_has_redirection()
{
	int i;
	for (i = 0; i < token_count; i++) {
		if (strcmp(tokens[i], "<") == 0) return 0;
		if (strcmp(tokens[i], ">") == 0) return 0;
	}
	return 1; // no redirection
}

/**
 * Check if redirection in the piped command is valid.
 */
int valid_redirection()
{
	int i, pipe_index = 0;
	
	// error checking.
	// shouldn't ever happen, we should have location of pipe already. 
	if ( (pipe_index = pipe_info[1]) <= 0) {
		return 1;
	}
	
	// check for redirection before pipe (only allowing '<' before pipe)
	for (i = 0; i < pipe_index; i++) {
		if (strcmp(tokens[i], ">") == 0) return 1;
	}

	// check for redirection after pipe (only allowing '>' after pipe).
	for (i = pipe_index + 1; i < token_count; i++) {
		if (strcmp(tokens[i], "<") == 0) return 1;
	}

	return 0; // valid redirection
}

/**
 * Determine if the program is a built in program.
 */
int is_builtin(char* prog)
{
	int i;
	for (i = 0; i < BUILTIN; i++) {
		if (strcmp(prog, builtin[i]) == 0) {
			return 0;
		}
	}
	return 1;
	
}

/**
 * Determine the type of program given through input.
 * Return -1 if not a valid program.
 */
int determine_prog_type(int index, char* prog)
{
	// return 1 for built in, 2 for needing to use exec.
	if (is_builtin(prog) == 0) return 1;
	if (!find_command(index)) return 2;
	return -1; // bad command
}

/**
 * Call programs with a pipe (and no redirection). 
 */
int simple_pipe()
{
	int pipe1_id, pipe2_id;
	int p[2];

	// start pipe
	if (pipe(p) < 0) {
		perror("pipe");
		return 1;
	}	

	// fork for first process
	if ( (pipe1_id = fork()) < 0) {
		perror("fork");
		return 1;
	}

	if (pipe1_id == 0) { // child process
		close(p[0]); // don't need the read end.
		dup2(p[1], STDOUT_FILENO); // duplicate STDOUT for second process
		close(p[1]);

		switch(determine_prog_type(0, tokens[0])) {
		case 1:
			break;
		case 2:
			if (execvp(tokens[0], args1) < 0) {
				perror("exec");
				return 1;
			}
		}
	}
	else { // parent process
		if ( (pipe2_id = fork()) < 0) {
			perror("fork");
			return 1;
		}
		// child 2 here
		if (pipe2_id == 0) {
			close(p[1]); // we only need read end.
			dup2(p[0], STDIN_FILENO); // dup read end
			close(p[0]);

			switch(determine_prog_type(pipe_info[1], tokens[pipe_info[1]])) {
			case 1:
				break;
			case 2:
				if (execvp(tokens[pipe_info[1]], args2) < 0) {
					perror("exec");
					return 1;
				}
			default:
				write(STDOUT_FILENO, "Invalid command!\n", 17);
				return 1;
			}
		}
		// parent executing, waiting for both children.
		else {
			wait(NULL);
			wait(NULL);
		}
	}

	return 0;
}

/**
 * Call a program containing a pipe.
 */
int call_program_with_pipe()
{
	
	// determine if there is any redirection
	if (valid_redirection() == 1) {
		write(STDOUT_FILENO, "Invalid redirection combined with pipe!\n", 40);
		return 1;
	}

	// determine where redirection takes place (if any).
/*	if (has_redirection_before() == 0) {
		return 0;
	}
	else if (has_redirection_after() == 0) {
		return 0;
	}
	else if (has_redirection_both() == 0) {
		return 0;
	}
	else {
		return simple_pipe();
*/	//}
	
	return simple_pipe();
}

/**
 * Check if the command redirects output somewhere else.
 */
int has_redirect_out()
{
	int i;
	for (i = 0; i < token_count; i++) {
		if (strcmp(tokens[i], ">") == 0) return 0;
	}
	return 1;
}

/**
 * Call program with output redirection.
 */
int handle_redirect_out()
{
	int res, fd, copy, id;

	// args2[0] = output file to send output to.
	if (is_builtin(tokens[0]) == 0) {
		copy = dup(STDOUT_FILENO);
		fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0640);	
		dup2(fd, STDOUT_FILENO);
		close(fd);
		res = handle_builtin(tokens[0], args1);
		dup2(copy, STDOUT_FILENO);
		close(copy);
		return res;
	} else {
		if (find_command(0) == 0) {
			copy = dup(STDOUT_FILENO);
			fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0640);
			dup2(fd, STDOUT_FILENO);
			close(fd);
			if ( (id = fork()) == 0) {
				//printf("|%s|\n", args1[1]);
				if (execvp(tokens[0], args1) < 0) {
					perror("exec");
				}
				return 1;
			}
			else {
				wait(NULL);
				dup2(copy, STDOUT_FILENO);
				close(copy);
				return 0;
			}
		}
	}
	
	write(STDOUT_FILENO, "Program does not exists.\n", 25);
	return 1;
}

// free mem from cpy
void free_cpy(char** c, int size)
{
	for (int i = 0; i < size; i++) {
		free(c[i]);
	}
	free(c);
}

/**
 * Search set of directories for given program name.
 */
int find_cmd(char** t, int index)
{
	int i, j, dir_count = 0, name_count = 0;
	char* str;
	struct stat *statbuf;
	statbuf = malloc(sizeof(struct stat));
	
	i = 0;
	while (t[index][i] != '\0') {
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
		memcpy(&(str[dir_count]), t[index], name_count);
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
 * Handle taking in input from file redirect.
 */
int handle_redirect_in()
{
	int res, fin, fd, copy, copy_count, id;
	
	fin = open(tokens[2], O_RDONLY);
	if (fin <= 0) {
		perror("open");
		return 1;
	}

	// make copy of tokens
	char** cpy = (char **) malloc(sizeof(char *) * token_count);
	
//	printf("tokens: %d\n", token_count);
	for (int i = 0; i < token_count; i++) {
		printf("size: %ld\n", sizeof(tokens[i]));
		cpy[i] = malloc(sizeof(tokens[i]));
		strcpy(cpy[i], tokens[i]);
	}
	copy_count = token_count;

	free_tokens();
	free_read_arr();
	token_count = 0;
	read_arr_size = 0;
	read_input(fin);
	tokenize_input();
	print_tokens();

	// args2[0] = output file to send output to.
	if (is_builtin(cpy[0]) == 0) {
		copy = dup(STDOUT_FILENO);
		fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0640);	
		dup2(fd, STDOUT_FILENO);
		close(fd);
		res = handle_builtin(cpy[0], args1);
		dup2(copy, STDOUT_FILENO);
		close(copy);
		free_cpy(cpy, copy_count);
		return res;
	} else {
		if (find_cmd(cpy, 0) == 0) {
			copy = dup(STDOUT_FILENO);
			fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0640);
			dup2(fd, STDOUT_FILENO);
			close(fd);
			if ( (id = fork()) == 0) {
				if (execvp(cpy[0], tokens) < 0) {
					perror("exec");
				}
				free_cpy(cpy, copy_count);
				return 1;
			}
			else {
				wait(NULL);
				dup2(copy, STDOUT_FILENO);
				close(copy);
				free_cpy(cpy, copy_count);
				return 0;
			}
		}
	}
	
	write(STDOUT_FILENO, "Program does not exists.\n", 25);
	free_cpy(cpy, copy_count);
	return 1;
}

/**
 * Check if we have file redirect in.
 */
int has_redirect_in()
{
	int i;
	for (i = 0; i < token_count; i++) {
		if (strcmp(tokens[i], "<") == 0) return 0;
	}
	return 1;
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
		return call_program_with_pipe();		
	}
	
	// check if there are any redirects (only redirects at this point)
	if (has_redirect_out() == 0) {
		return handle_redirect_out();
	}
	if (has_redirect_in() == 0) {
		return handle_redirect_in();
	}


	// check if we have a build in command
	switch (handle_builtin(tokens[0], args1)) {
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
 * Check tokens if home directory escape sequence occurs, inserts path if it does.
 */
void check_home()
{
	int i, j, len = 0, token_len = 0;
	char* env;
	char buff[100];
	char cpy[100];

	for (i = 0; i < token_count; i++) {
		if (tokens[i][0] == '~') {
			env = getenv("HOME");
			if (env == NULL) return;
			for (j = 0; j < 100; j++) {
				if (env[j] == '\0') break;
				buff[j] = env[j];
				len++;
			}
			// get size of token
			for (j = 0; j < 100; j++) {
				token_len++;
				if (tokens[i][j] == '\0') break;
			}
			memcpy(cpy, &(tokens[i][1]), token_len-1);
			char* copy = realloc(tokens[i], (token_len + len + 1));
			if (copy == NULL) return;
			memcpy(copy, buff, len);
			memcpy(&(copy[len]), cpy, token_len-1);
			tokens[i] = copy;
		}
	}
}

/**
 * Expand wildcard if there are matches, return 0 if no matches.
 */
int expand_wildcard(char* str, int length)
{
	//printf("String: %s\n", str);

	int j, wildcard_hit, len, before_len, after_len, difference, match;
	char before[50]; // before wildcard
	char after[50];  // after wildcard
	char buff[100];
	char cpy[50];
	struct dirent *de;
	
	DIR *dp = opendir(getcwd(buff, 100));

	if (!dp) {
		perror("opendir");
		return 0;
	}

	wildcard_hit = 0, before_len = 0, after_len = 0, j = 0;	
	while (str[j] != '\0') {
		if (str[j] == '*') {
			wildcard_hit = 1;
			j++;
			continue;
		}
		if (wildcard_hit) {
			after[after_len] = str[j];
			after_len++;
		}
		else {
			before[before_len] = str[j];
			before_len++;
		}
		j++;
	}
		
	before[before_len] = '\0';
	after[after_len] = '\0';
	//before_len;
	//after_len;
	//printf("before: %s, after: %s\n", before, after);
	match = 0;
	// search the directory for matches
	while ( (de = readdir(dp)) ) {
		len = strlen(de->d_name);
		strncpy(cpy, (de->d_name), before_len);
		cpy[before_len] = '\0';
		if (strcmp(cpy, before) == 0) {
			memset(cpy, '\0', 50);
			strncpy(cpy, &( (de->d_name)[len - after_len] ), after_len);
			if (strcmp(cpy, after) == 0) {
				// we have a wildcard match, add a new token.
				if (token_count + 2 >= tokens_capacity) {
					tokens_capacity += tokens_capacity;
					tokens = (char **) realloc(tokens, (sizeof(char *) * tokens_capacity));
				}
				tokens[token_count] = (char *) malloc(sizeof(char) * (len + 1));
				memcpy(tokens[token_count], (de->d_name), len+1);
				token_count++;
				match = 1;
			}
			memset(cpy, '\0', 50);
		}
	}
	closedir(dp);
	return match;
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
		if (tokenize_input()) {
			write(STDOUT_FILENO, "No wildcard match!\n", 19);
			status = 1;
			free_tokens();
			free_read_arr();
			continue;
		}
	//	print_tokens();
		// do something with input
		check_home();
		split_args();
		//print_args();

		//expand_wildcard();
//		print_tokens();
		status = call_program();
		
		// free read array (and eventually tokens array
		free_read_arr();
		free_tokens();
		free_args();
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

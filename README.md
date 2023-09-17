Authors: 
	Lucas Souders - las541
	Sage Thompson - st1039


Our Process for Bulding our shell
=======================================================================================================
Handling input:

To start, the biggest task of creating our shell was being able to read in input through either STDIN
or a given file, and then tokenize it. Our approach was to simply read one character at a time to a 
fixed-size buffer, and then send this buffer to an array of strings once it was filled. Once we 
encountered a new-line character, we stopped reading and sent what we read to our tokenizer. Now in 
the tokenizer we continued to read through our large string(s) of input one character at a time, until
we reached that same new-line character again. We split up our tokens whenever we encountered one of:
{' ', '<', '>', '|'}, since these are the only characters that delimit one token from another. We then
dynamically build an array of tokens, where each token is null terminated (which means each token can
be handled as a string).

Using our tokens:
=========================================================================================================
Once our tokenizer was complete, we now have an array of tokens. From here, we must decide how our 
command is going to behave. Is there any redirection? Is there a pipe? Is there a combination of both?
Aside from changes in flow, we also need to figure out what type of program we need to run. To elaborate,
Are we calling a function we built in to our shell (i.e., cd, pwd, exit)? Are we calling a system function
with exec()? Are we calling an executable (denoted ./program\_name)? These are all things we need to
consider, which we will handle through various helper functions.

Piping:
=========================================================================================================
If there is any piping taking place, we must create subprocesses to handle writing output to our next
program, where that program will read it in and make use of it. Also, it is worth noting that if piping
and redirection is combined into one command, it will be handled as follows:
- '<' will only occur before a pipe (as input to the first program). If this occurred after the
	pipe, we would have two sources of input for the second program, which is an issue.
- '>' will only occur after a pipe. If this had occurred before the pipe, we would essentially 
	be sending output to the second program twice, which is an issue.
If a pipe and redirection is not applied as stated above, the command will be considered invalid and 
an error will be returned.
(Piping is still currently being worked on as we've been having some implementation issues.)

Redirection:
=========================================================================================================
Redirecting output: To do this, we read the first token after ">" in our token list. We then duplicated
the the given text file with STDOUT\_FILENO to set a new file descriptor for us to send program output
to. We are still currently working on file input. 

Wildcards:
=========================================================================================================
To handle wildcard expansion, we kept a flag indicating if a wildcard was found while tokenizing our 
input. If this were the case, then we used a helper function to open the current working directory, and
search for matching files. To do this, we stored the characters before and after the * in our token. These
were stored as strings, which we kept track of the length of. Then, we used strcmp to compare the 
before_\*_str with dir\_entry\_str upto before\_\*\_str\_len. We did the same for after the * . If both 
matched, we had a wildcard expansion and added it as a token. 

Extensions:
=========================================================================================================
We completed extensions (3.1) Escape sequences and (3.2) Home directory. To handle escape sequences,
whenever we encountered a backslash when building our tokens, we simply added the next character to the
token we were building, regardless of what that character was. For the home directory, we simply used
getenv("HOME") to get a string that gave an absolute path to the home directory. We then took whatever 
argument that contained '~/', and concatonated anything after the forward slash onto the string that
represented the home directory path. For example, if the home directory is /common/home/las541, and we
have a token that is ~/Desktop/SoftwareMethodology, the token would be changed to:
/common/home/las541/Desktop/SoftwareMethodology. Additionally, if cd was called with no arguments,
we got the home environment string and passed it to chdir().

Testing:
==========================================================================================================
A majority of our testing involved testing the tokenizer with various different commands, some including 
spaces between tokens, some not. We then have a print\_tokens() in our program to display the tokens so
we can verify that they are all store properly. Some test cases are as follows:
- echo Hello World > from < our | shell?
- cat some >test command> for our|tokenizer
- cd this<is>not|a directory><
- foo bar\ baz\<qu\ux\\>spam (this was used to verify extension 3.1)

We then needed to test that we can run programs with the proper argument lists. Some of our test cases are:
- echo Hello World > foo (foo must be created if it doesn't exist, truncated if it does).
- cat foo	         (must print "Hello World").
- vim mysh.c		 (We were able to work on our program while inside our program, which we found really cool).
- make			 (The program compiled while in our program, but obviously we needed to terminate and run again
				to see the changes in effect).
- ls ~/Desktop > foo	 (verify that ls is working, along with extension 3.2 and output redirection).
- touch food friend flipped
- echo f\*d	  	 (Make sure wildcard expansion is working. This should print: food friend flipped (in no certain order).
- mkdir frye fine, then touch foo in frye and touch bar in fine 
- ls f\*e > output	 (this will print the contents of directories 'frye' and 'fine' into a file named output) 
- exit			 (make sure we can terminate properly, especially without memory leaks)
- cd			 (verify we can cd to home directory when given no arguments).


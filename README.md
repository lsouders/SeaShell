Authors: 
	Lucas Souders - las541
	Sage Thompson - st1039


Our Process for Bulding our shell
======================================================================================================
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

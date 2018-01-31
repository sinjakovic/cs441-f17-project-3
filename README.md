# CS441/541 Project 3

## Author(s):

Brandon Sinjakovic


## Date:

10/13/2017


## Description:

This is an implementation of a commnand line interpreter or shell. The shell is 
able to handle running multiple processes simultaneously in the foreground and
background. The shell also able to run in two different modes: interactive and batch.
jobs will either run sequentially or in parallel as determined by job separators.


## How to build the software

This program uses a makefile to compile with the command 'make'.


## How to use the software

After building the software, to use run the program in interactive mode, use the
command './mysh'. to quit, use the command 'exit' or CTRL-D. 
To run the program in batch mode, use the command './mysh text_file' where
text_file is a text file with a list of jobs.


## How the software was tested

The software was tested using the given tests along with tests made by the authors

my_test1.txt: This test is to test the count and wait on all background jobs before exiting
my_test2.txt: This test is to test the 'fg' command when given a valid job id.
my_test3.txt: This test is to test the 'fg' command when given an invalid job id and should return an error.
my_test4.txt: This test is to test the 'wait' job when there are jobs in the background
my_test5.txt: This test is to test the file redirection function.


## Known bugs and problem areas

No known bugs or problem areas.

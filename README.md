# Simple Shell

## Overview
This project is a simple implementation of a computer's shell. It is capable of running commands with or without arguments. It supports output and error redirection, and basic environmental variables. It also supports piping, using the standard fork-exec-wait process.

### Restrictions
The entire input command must at most 512 characters.
Each argument must be at most 32 characters.
The number of arguments for a command must be at most 16, including the command itself.
The commands *exit*, *cd*, *set*, and *pwd* cannot be used in piping or have their output or error redirected.
If error is redirected, output must also be redirected.
Output and error must be redirected to the same file.

## Structure
The shell follows a straightforward set of steps: accepting user input, parsing, piping and forking, and running the commands.

### Accepting the command
The *main()* function is a loop that runs until the user inputs *exit* as a command. The first step of the loop is to store the file descriptors of *stdout* and *stderr* using *dup()* so that they may be restored at the end of the loop if the user redirects either to a file. Then, the program prompts the user to enter a command and stores the result. The next phase, which is the bulk of the code, is parsing the command line.

### Output Redirection
The first step of parsing is to find any output or error redirection. The program first checks for the token '&>', which will redirect both *stdout* and *stderr* to the file specified after the token. If it is not found, it will then check for '>', which will redirect just *stdout*. If either token is found, the appropriate redirections are made using *dup2()*. The enum *redirect_status* is used to store what kind of redirect was used. The rest of the command line is isolated from the redirect token and used in the rest of the parsing.

### Pipe and Arg Parsing
The next step is to store every command separated by a '|' as a string in a 2D array. The program uses *strtok()* to separate commands between vertical bars and fill in the 2D array. For each of these commands, *strtok()* is used again to isolate each argument. If the user passed in an environmental variable, it is here that the program recognizes it and replaces it with its corresponding value.

### Command Structs
Once each command on the line is isolated, the program then transfers them into an array of Command structs. Each Command struct contains the main command (eg “ls” in “ls -l”), stored as a char pointer, its arguments, and its number of arguments, stored as an unsigned int. The args in each struct are dynamically allocated and stored as a char double pointer to make them compatible with *execvp*, which is used to run the commands.

### Piping and Forking
The next phase is to actually run the commands. Because the program assumes that the built-in commands are run in isolation, it checks them before it pipes and forks. For all others, for *n* commands, it creates *n-1* pipes and *n* forks. Each fork changes its output to the pipe’s output, and its input to the input of the previous pipe. The exceptions are the first and last pipe, which do not require redirecting input and output, respectively. The commands are finally ready to be run with a simple *execvp*. The members of each Command struct can easily be passed in as the arguments. Meanwhile, the parent process waits for every child process (individual commands) to finish.

### End
The final phase begins with de-allocating the memory set aside for the args for each command. The parent process waits for every child to finish, return *stdout* and *stderr* to normal if necessary, and prints the return value of the process.

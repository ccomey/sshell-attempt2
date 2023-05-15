#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define TOKEN_MAX 32
#define MAX_ARGS 16

struct Command{
    char main_command[TOKEN_MAX];
    unsigned num_args;
    char** args;
};


int main(){

    char cmd[CMDLINE_MAX];

    while (1){
        char* nl;
        // int retval = 1;

        // print prompt
        printf("sshell$ ");
        fflush(stdout);

        // get command line
        fgets(cmd, CMDLINE_MAX, stdin);

        // print command line if stdin is not provided by terminal
        if (!isatty(STDIN_FILENO)){
            printf("%s", cmd);
            fflush(stdout);
        }

        // remove trailing newline from command line
        nl = strchr(cmd, '\n');
        if (nl){
            *nl = '\0';
        }
    }

    return 0;
}
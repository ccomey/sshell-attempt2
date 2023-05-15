#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

/**
 * removes leading and trailing whitespace
 * removes leading whitespace by incrementing the pointer until it reaches a non-space
 * removes trailing whitespace by placing a null terminator on the first trailing whitespace
*/
char* stripWhiteSpace(char* str){
        // remove leading whitespace
        for (unsigned i = 0; str[0] == ' '; i++){
                str++;
        }

        // remove trailing whitespace
        char* str_end = strchr(str, '\0');
        for (unsigned i = 0; i < (str_end - str); i++){
                if (*(str_end-i) != ' ' && *(str_end-i) != '\0'){
                        *(str_end-i+1) = '\0';
                        break;
                }
        }

        return str;
}

bool parseOutputRedirection(const char* cmd_line, char* core_command, char* output_file){
    char* output_token = strstr(cmd_line, ">");
    // printf("line1\n");

    strcpy(core_command, cmd_line);

    if (!output_token){
        output_file = NULL;
        return false;
    }

    // printf("%p - %p = %ld\n", output_token, cmd_line, (output_token-cmd_line));
    core_command[output_token-cmd_line] = '\0';
    // printf("line3\n");

    strcpy(output_file, cmd_line);
    // printf("output_token+1 = %s\n", (output_token+1));
    char* output_file_retval = stripWhiteSpace(output_token+1);
    strcpy(output_file, output_file_retval);
    // printf("output_file = %s\n", output_file);
    return true;
}


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

        char core_command[sizeof(cmd)];
        char output_file[sizeof(cmd)];

        parseOutputRedirection(cmd, core_command, output_file);
        printf("core command: %s\noutput file: %s\n", core_command, output_file);
    }

    return 0;
}
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

struct Command* initCommand(void){
    struct Command* command = malloc(sizeof(struct Command));
    command->main_command[0] = '\0';
    command->num_args = 0;
    command->args = malloc(MAX_ARGS * sizeof(char*));

    for (unsigned i = 0; i < MAX_ARGS; i++){
        command->args[i] = malloc(TOKEN_MAX * sizeof(char));
        memset(command->args[i], '*', TOKEN_MAX * sizeof(char));
    }

    return command;
}

bool destroyCommand(struct Command* command){
    if (command == NULL){
        return false;
    }

    for (unsigned i = 0; i < MAX_ARGS; i++){
        free(command->args[i]);
    }


    free(command->args);
    free(command);
    return true;
}

void printCommandStruct(struct Command* cmd){
    printf("main command: %s\n", cmd->main_command);
    for (unsigned i = 0; i < cmd->num_args; i++){
        printf("arg%d: %s\n", i, cmd->args[i]);
    }
}

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

bool parseArgs(const char* cmd_line, struct Command* command){


    char cmd_copy_array[sizeof(cmd_line)];
    strcpy(cmd_copy_array, cmd_line);
    char* cmd_copy = &cmd_copy_array[0];
    printf("set up cmd_copy\n");

    // command is the first word in cmd
    // ex: in $ ls -l, ls is the main_command
    strcpy(command->main_command, strtok(cmd_copy, " "));
    // printf("main command = %s\n", command->main_command);

    // the first arg is the command itself
    strcpy(command->args[0], command->main_command);
    (command->num_args) = 1;

    // for every other arg, add every word in the rest of cmd as an element of args
    for (unsigned i = 1; ; i++){
            char* tok = strtok(NULL, " ");
            // printf("tokenized arg\n");

            if (tok != NULL && command->num_args == MAX_ARGS){
                    fprintf(stderr, "too many process arguments\n");
                    exit(-1);
            }

            // the last arg should always be null
            // so if it is, break
            if (tok == NULL){
                    // strcpy doesn't work with NULL, so it must be set directly
                    command->args[i] = NULL;
                    (command->num_args)++;
                    break;
            } else {
                    strcpy(command->args[i], tok);
                    (command->num_args)++;
            }
    }

    return command->num_args > 1;
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

        struct Command* cmd1 = initCommand();

        destroyCommand(cmd1);
    }

    return 0;
}
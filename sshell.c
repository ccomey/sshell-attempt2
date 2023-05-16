#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // needed for flags in open()

#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define TOKEN_MAX 32
#define MAX_ARGS 16

int mallocs = 0;
long int bytes = 0;

// each command on a line (multiple can be input using piping)
// args has to be dynamically allocated
struct Command{
    char main_command[TOKEN_MAX];
    unsigned num_args;
    char** args;
};

// dynamically allocate and initialize a command struct
struct Command* initCommand(void){
    struct Command* command = malloc(sizeof(struct Command));
    bytes += sizeof(struct Command);
    mallocs++;

    // memset is used to avoid potentially buggy chars like unintended null terminators
    memset(command->main_command, '*', TOKEN_MAX * sizeof(char));
    command->num_args = 0;
    command->args = malloc(MAX_ARGS * sizeof(char*));
    mallocs++;
    bytes += MAX_ARGS * sizeof(char*);

    for (unsigned i = 0; i < MAX_ARGS; i++){
        command->args[i] = malloc(TOKEN_MAX * sizeof(char));
        memset(command->args[i], '*', TOKEN_MAX * sizeof(char));
        mallocs++;
        bytes += TOKEN_MAX * sizeof(char);
    }

    // printf("initCommand: %d mallocs\n", mallocs);
    // printf("initCommand: %ld bytes\n", bytes);
    return command;
}

// deallocate a command struct
// returns false if the parameter is null, or true otherwise
bool destroyCommand(struct Command* command){
    if (command == NULL){
        return false;
    }

    for (unsigned i = 0; i < MAX_ARGS; i++){
        bytes -= sizeof(command->args[i]);
        free(command->args[i]);
        mallocs--;
    }

    bytes -= sizeof(command->args);
    free(command->args);
    mallocs--;
    bytes -= sizeof(command);
    free(command);
    mallocs--;
    // printf("destroyCommand: %d mallocs\n", mallocs);
    // printf("destroyCommand: %ld bytes\n", bytes);
    return true;
}

// print the command's args
void printCommandStruct(struct Command* cmd){
    printf("main command: %s\n", cmd->main_command);
    for (unsigned i = 0; i < cmd->num_args; i++){
        printf("arg%d: %s\n", i, cmd->args[i]);
    }
}

// set up the file redirection using dup2
// will redirect output, or output and error, or neither
void redirectOutput(const char* filename, const bool redirecting_output, const bool redirecting_error){
        // printf("running redirectOutput\n");

        // printf("%d %d\n", redirecting_output, redirecting_error);

        if (!redirecting_output){
            return;
        }

        // the flags are as follows
        //make destFile write-only
        // create destFile with read and write permissions if it does not exist
        // truncate the file's contents if it is longer than its new contents
        int destFile = open(filename, O_WRONLY | O_CREAT, 0666 | O_TRUNC, 0);
        dup2(destFile, STDOUT_FILENO);

        // if we are redirecting stderr, redirect that to destFile too
        if (redirecting_error){
                dup2(destFile, STDERR_FILENO);
        }

        close(destFile);
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

// parse the command line for the output token (>& if redirecting output and error, > if redirecting only output, and none otherwise)
// core_command is set to the part of the command line before the token, and output file to what comes after
bool parseOutputRedirection(const char* cmd_line, char* core_command, char* output_file, const bool redirect_error){
    char* output_token = redirect_error ? ">&" : ">";
    char* ptr_to_output_token = strstr(cmd_line, output_token);
    // printf("line1\n");

    strcpy(core_command, cmd_line);

    if (!ptr_to_output_token){
        output_file = NULL;
        return false;
    }

    // printf("%p - %p = %ld\n", output_token, cmd_line, (output_token-cmd_line));
    core_command[ptr_to_output_token-cmd_line] = '\0';
    // printf("line3\n");

    strcpy(output_file, cmd_line);
    // printf("output_token+1 = %s\n", (output_token+1));
    char* output_file_retval = stripWhiteSpace(ptr_to_output_token+strlen(output_token));
    strcpy(output_file, output_file_retval);
    // printf("output_file = %s\n", output_file);
    return true;
}

void parseForPipeBars(char* cmd_without_redirect, char commands[TOKEN_MAX][TOKEN_MAX], unsigned* num_commands){
    // load each command between the | into commands
    // the first is done separately because strtok requires different arguments
    char* command1 = strtok(cmd_without_redirect, "|");
    stripWhiteSpace(command1);
    strcpy(commands[0], command1);
    *(num_commands) = 1;

    // add the rest of the commands to commands
    for (unsigned i = 1; ; i++){
            char* tok = strtok(NULL, "|");

            if (tok == NULL){
                    break;
            }
            
            tok = stripWhiteSpace(tok);
            strcpy(commands[i], tok);
            *num_commands += 1;
    }
}

// parses a command for args and inserts them into a struct
bool parseArgs(const char* cmd_line, struct Command* command){


    char cmd_copy_array[strlen(cmd_line) * sizeof(char)];
    strcpy(cmd_copy_array, cmd_line);
    char* cmd_copy = &cmd_copy_array[0];
    // printf("set up cmd_copy\n");

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
        int retvals[CMDLINE_MAX];
        memset(retvals, 0, CMDLINE_MAX * sizeof(int));

        int stdout_backup = dup(STDOUT_FILENO);
        int stderr_backup = dup(STDERR_FILENO);
        enum redirect_statuses{NONE, OUT, OUT_AND_ERR} redirect_status;

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

        if (parseOutputRedirection(cmd, core_command, output_file, true)){
            redirect_status = OUT_AND_ERR;
        } else {
            if (parseOutputRedirection(cmd, core_command, output_file, false)){
                redirect_status = OUT;
            } else {
                redirect_status = NONE;
            }
        }

        // printf("redirect status = %d\n", redirect_status);
        redirectOutput(output_file, !(redirect_status == NONE), redirect_status == OUT_AND_ERR);

        // printf("core command: %s\noutput file: %s\n", core_command, output_file);

        // commands stores each command separated by a |, separately
        char commands[TOKEN_MAX][TOKEN_MAX];
        stripWhiteSpace(core_command);

        // the command should not start or end with a |, so reset if so
        if (core_command[0] == '|'){
                fprintf(stderr, "Error: missing command\n");
                // restart_command_line_input = true;
                continue;
        } else if (*(strchr(core_command, '\0')-1) == '|'){
                fprintf(stderr, "Error: missing command\n");
                // restart_command_line_input = true;
                continue;
        }


        // pipe parsing phase
        unsigned num_commands = 0;
        parseForPipeBars(core_command, commands, &num_commands);

        // command_structs stores an array of Command structs
        struct Command* command_structs[TOKEN_MAX];

        // fill up command_structs
        for (unsigned i = 0; i < num_commands; i++){
            // allocate memory for the args for each command
            command_structs[i] = initCommand();

            // fill the members of each struct
            parseArgs(commands[i], command_structs[i]);
            // printCommandStruct(command_structs[i]);
        }

        if (num_commands == 1 && strcmp(command_structs[0]->main_command, "exit") == 0){
            destroyCommand(command_structs[0]);
            printf("Bye...\n");
            break;
        }


        pid_t PIDs[num_commands];
        int FDs[num_commands-1][2];
        // printf("num_commands = %d\n", num_commands);

        for (unsigned i = 0; i < num_commands; i++){
            if (i < num_commands-1){
                pipe(FDs[i]);
                // printf("FDs[%d] = %d,%d\n", i, FDs[i][0], FDs[i][1]);
            }

            if (!(PIDs[i] = fork())){
                // printf("%d running command %s\n", i, command_structs[i]->main_command);

                if (num_commands == 1){

                } else {
                    if (i > 0){
                        close(FDs[i-1][1]);
                        // printf("attempting to access FD[%d][0] = %d\n", i-1, FDs[i-1][0]);
                        dup2(FDs[i-1][0], STDIN_FILENO);
                        close(FDs[i-1][0]);
                    }
                    
                    if (i < num_commands-1){
                        close(FDs[i][0]);
                        // printf("attempting to access FD[%d][1] = %d\n", i, FDs[i][1]);
                        dup2(FDs[i][1], STDOUT_FILENO);
                        close(FDs[i][1]);
                    }
                }

                for (unsigned j = 0; j < num_commands-1; j++){
                    if (j != i && j != i-1){
                        close(FDs[j][0]);
                        close(FDs[j][1]);
                    }
                }

                retvals[i] = execvp(command_structs[i]->main_command, command_structs[i]->args);
                if (retvals[i] != 0){
                    fprintf(stderr, "command not found\n");
                }

                exit(retvals[i]);
            }
        }

        // close the FDs in the parent
        for (unsigned i = 0; i < num_commands-1; i++){
            close(FDs[i][0]);
            close(FDs[i][1]);  
        }

        // wait for every child to finish
        for (unsigned i = 0; i < num_commands; i++){
            waitpid(PIDs[i], NULL, 0);
        }

        // reset output and error back to normal if they were changed
        if (redirect_status == OUT || redirect_status == OUT_AND_ERR){
            dup2(stdout_backup, STDOUT_FILENO);
        }

        if (redirect_status == OUT_AND_ERR){
            dup2(stderr_backup, STDERR_FILENO);
        }

        for (unsigned i = 0; i < num_commands; i++){
            destroyCommand(command_structs[i]);
        }
        
        printf("+ completed '%s' ", cmd);
        for (unsigned i = 0; i < num_commands; i++){
            printf("[%d]", retvals[i]);
        }

        printf("\n");
    }

    return 0;
}
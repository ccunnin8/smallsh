#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_LENGTH 2048

struct command {
    char *command;
    struct argument *arguments;
    char *inputFile;
    char *outputFile;
    bool background;
    int numArguments;
};

struct argument {
    char *argument;
    struct argument *nextargument;
};

void checkInputEnding(char userInput[]) {
    // removes a space or new line character from 

    int len = strlen(userInput) - 1;
    while (userInput[len] == ' ' || userInput[len] == '\n') {
        userInput[len] = '\0';
        len--;
    }
}

void freeList(struct argument *head) {
    while (head != NULL) {
        struct argument *tmp = head;
        head = head->nextargument;
        free(tmp);
    }
}

struct command *parseInput(char *userInput) {
    struct command *cmd = malloc(sizeof(struct command));
    cmd->command = NULL;
    cmd->arguments = NULL;
    cmd->inputFile = NULL;
    cmd->outputFile = NULL;
    cmd->background = false;
    cmd->numArguments = 0;

    struct argument *arg = malloc(sizeof(struct argument));
    arg->argument = NULL;
    arg->nextargument = NULL;
    struct argument *argPtr = arg; 

    // store arguments to parse 
    char arguments[MAX_LENGTH] = { 0 };
    int argumentCount = 1;

    // remove new line from userInput 
    checkInputEnding(userInput);

    // get first argument passed to scanf
    char *saveptr;
    char *token = strtok_r(userInput, " ", &saveptr);
    
    cmd->command = token;

    while (token = strtok_r(NULL, " ", &saveptr)) {
        if (argumentCount == 1) {
            arg->argument = token;
            argumentCount++;
        } else {
            if (strcmp("<", token) == 0) {
                // process stdin redirect 
                char *fileName = strtok_r(NULL, " ", &saveptr);
                cmd->inputFile = fileName;
            } else if (strcmp(">", token) == 0) {
                // process stdout redirect 
                char *fileName = strtok_r(NULL, " ", &saveptr);
                cmd->outputFile = fileName;
            } else if (strcmp("&", token) == 0){
                // process run in background 
                printf("ampersand!");
                fflush(stdout);
                cmd->background = true;
            } else {
                // process argument 
                struct argument *newarg = malloc(sizeof(struct argument));
                newarg->argument = token;
                newarg->nextargument = NULL; 

                argPtr->nextargument = newarg;
                argPtr = newarg;
                argumentCount++;
            }
        }
    }
    // add number of arguments to the command struct if there are arguments 
    if (arg->argument != NULL) {
        cmd->numArguments = argumentCount;
        cmd->arguments = arg; 
    }

    return cmd;
}

void expandInput(char input[], char newInput[]) {
    // expands instances of $$ to be pid 

    pid_t pid = getpid();

    // source: https://stackoverflow.com/questions/53230155/converting-pid-t-to-string
    // allocate enough memory for pid and cast to char *
    // use sprintf to write pid value into pidStr 

    char *pidStr = (char *)(malloc(sizeof(pid_t)));
    sprintf(pidStr, "%d", pid);
   
    bool firstSymbol = false;
    
    int j = 0;
    // loop through old input look for $
    for (int i = 0; i < strlen(input); i++) {
        // found $
        if (input[i] == '$') {
            // check if second $ and if so expand 
            if (firstSymbol) {
                strcat(newInput, pidStr);
                 j += strlen(pidStr);
                firstSymbol = false;
            // first $
            } else {
                firstSymbol = true;
            }
        // not a $
        } else {
            // reset search param for $
            if (firstSymbol) {
                firstSymbol = false;
                newInput[j] = '$';
                newInput[j + 1] = input[i];
                j++;
            } else {
               newInput[j] = input[i]; 
            }
            j++;
        }
    }
    free(pidStr);
}

void processInput(struct command *input) {
    // looks at the command in the struct given and executes 
    // built in function OR uses exec to execute non built in 
    if (strcmp(input->command, "exit") == 0) {
        // exit 
        // get group process id and kill all children 
        pid_t pgid = getgid();
        kill(pgid, 0);

        // exit program 
        exit(0);
    } else if (strcmp(input->command, "cd") == 0) {
        // change directory 
        
        // if no argument PWD = HOME  
        if (input->arguments == NULL) {
            char *home = getenv("HOME");
            int changePath = chdir(home);
            if (changePath == -1) {
                perror("An error occurred changing paths");
                fflush(stdout);
            }
        // change PWD to argument given
        } else {
            // check if directory exists?
            char *newPath = input->arguments->argument;
            int changePath = chdir(newPath);
            if (changePath == -1) {
                perror("Unable to change to that directory");
                fflush(stdout);
            }
        }

    } else if (strcmp(input->command, "status") == 0) {
        // get status  
        printf("Will print status\n");
        fflush(stdout);
    } else {
        // for a new process 
        // adpated from exploration: process api: executing a new program 
        pid_t spawnPid = fork();
        int childStatus;
        char *args[input->numArguments + 1];

        switch (spawnPid) {
            case -1:
                perror("fork error\n");
                exit(1);
                break;
            case 0:
                // use exec family 
                args[0] = input->command;
                struct argument *curr = input->arguments;
                int counter = 1;

                while (curr != NULL) {
                    args[counter] = curr->argument;
                    counter++;
                    curr = curr->nextargument;
                }

                args[counter] = NULL;
                
                int status = execvp(args[0], args);

                if (status == -1) {
                    perror("A problem occurred executing\n");
                    fflush(stdout);
                }
                exit(0);
                break;
            default:
                spawnPid = waitpid(spawnPid, &childStatus, 0);


        }
    }
}

int main(void) {
    while (1) {
        fflush(stdout);  
        char userInput[MAX_LENGTH];
        printf(": ");
        fflush(stdout); 
        fgets(userInput, MAX_LENGTH, stdin);
        fflush(stdin);
        // if comment or blank line ignore all input
        if (userInput[0] != '#' && strcmp(userInput,"\n") != 0) {
            char *expandedInput = calloc(strlen(userInput), strlen(userInput) * sizeof(char));
            expandInput(userInput, expandedInput);
            struct command *input = parseInput(expandedInput);
            processInput(input);
            // struct argument *args = input.arguments;
            fflush(stdin);
            fflush(stdout); 

            // free memory used 
            freeList(input->arguments);
            free(input);  
        }
    }
    return 0;
}
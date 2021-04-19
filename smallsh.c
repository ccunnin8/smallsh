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

// ******* RESIZEABLE ARRAY OF PID_T ************ /// 
pid_t *createPidArray() {
    pid_t *array = calloc(5, sizeof(pid_t) * 5);  
    return array;  
}

int getLengthArray(pid_t *arr) {
  int index = 0;
  while (arr[index] != 0) index++;
  return index;
}

void insertIntoArray(pid_t *arr, pid_t val, int len) {
    
    int index = 0;
    while (arr[index] != 0) {
        index++;
    }
    if (index > len / 2) {
        int newLen = len * 2;
        pid_t *newArray = calloc(newLen, sizeof(pid_t) * newLen);
        for (int i = 0; i < len; i++) {
            newArray[i] = arr[i];
        }
        arr = newArray;
    }
    arr[index] = val;
}

void removeFromArray(pid_t *arr, pid_t val, int len) {
    int index = 0;
    while (arr[index] != val && index < len) index++; 
    if (arr[index] == val) {
        arr[index] = 0;
        if (index < len - 2) {
            while (index < len) {
                arr[index] = arr[index + 1];
                index++;
            }
        }
    }
}
// *****************************************************

// ********** COMMAND and ARG STRUCT ***************************
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

// ********************************************************

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
            cmd->background = true;
        } else {
            if (argumentCount == 1) {
                arg->argument = token;
                argumentCount++;
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

void redirectInput(char inputFile[]) {
    // redirects stdin to be file given 
    int file = open(inputFile, O_RDONLY);
    if (file == -1) {
        perror("File not found");
        exit(1);
    } else {
        int redirect = dup2(file, 0);
        if (redirect == -1) {
            perror("Error redirecting stdin to file");
            exit(1);
        }
    }
}

void redirectOutput(char outputFile[]) {
    // redirects stdout to be file given 
    int file = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        perror("Error opening/creating file");
        exit(1);
    } else {
        int redirect = dup2(file, 1);
        if (redirect == -1) {
            perror("Error redirecting to file");
            exit(1);
        }
    }
}
void processInput(struct command *input, int *currentStatus, pid_t *bgPids) {
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
            }
        // change PWD to argument given
        } else {
            // check if directory exists?
            char *newPath = input->arguments->argument;
            int changePath = chdir(newPath);
            if (changePath == -1) {
                perror("Unable to change to that directory");
            }
        }

    } else if (strcmp(input->command, "status") == 0) {
        // get status  
        printf("exit value %d", *currentStatus);
        fflush(stdout);
    } else {
        // for a new process 
        // adpated from exploration: process api: executing a new program 
        pid_t spawnPid = fork();
        char *args[input->numArguments + 1];
        int childStatus;
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
                
                // redirect stdout if file given
                if (input->outputFile != NULL) {
                    redirectOutput(input->outputFile);
                }

                // redirect stdin if file given 
                if (input->inputFile != NULL) {
                    redirectInput(input->inputFile);
                }
                
                int status = execvp(args[0], args);

                if (status == -1) {
                    perror("A problem occurred executing");
                    exit(1);
                }

                exit(0);
                break;
            default:
                // if in background add to bgPids list 
                if (input->background) {
                    printf("background pid is %d\n", spawnPid);
                    insertIntoArray(bgPids, spawnPid, getLengthArray(bgPids));
                }

                spawnPid = waitpid(spawnPid, &childStatus, input->background ? WNOHANG : 0);
               
                *currentStatus = WEXITSTATUS(childStatus);
        }
    }
}

int main(void) {
    int status = 0;
    pid_t *bgPids = createPidArray();

    while (1) {
        fflush(stdout);  
        char userInput[MAX_LENGTH];

        // loop through bgPids, if exited print msg and remove from bgPids
                int index = 0;
                while (bgPids[index] != 0) {
                    int bgStatus;
                    pid_t bgPid = waitpid(bgPids[index], &bgStatus, WNOHANG);
                    if (bgPid != 0) {
                        if (WIFEXITED(bgStatus)) {
                            printf("background pid %d is done: exit value: %d\n", bgPids[index], WEXITSTATUS(bgStatus));
                            removeFromArray(bgPids, bgPids[index], getLengthArray(bgPids));
                        }
                    }
                    index++;
                }
        printf(": ");
        fflush(stdout); 
        fgets(userInput, MAX_LENGTH, stdin);
        fflush(stdin);
        // if comment or blank line ignore all input
        if (userInput[0] != '#' && strcmp(userInput,"\n") != 0) {
            char *expandedInput = calloc(strlen(userInput), strlen(userInput) * sizeof(char));
            expandInput(userInput, expandedInput);
            struct command *input = parseInput(expandedInput);
            processInput(input, &status, bgPids);
            // struct argument *args = input.arguments;
            fflush(stdin);
            fflush(stdout); 

            // free memory used 
            freeList(input->arguments);
            free(input);  
            free(expandedInput);
        }
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LENGTH 2048

struct command {
    char *command;
    struct argument *arguments;
    char *inputFile;
    char *outputFile;
    bool background;
};

struct argument {
    char *argument;
    struct argument *nextargument;
};

struct command parseInput(char *input) {
    struct command cmd= { NULL, NULL, NULL, NULL, false };
    struct argument arg = { NULL, NULL };
    struct argument *argPtr = &arg; 

    // store arguments to parse 
    char arguments[MAX_LENGTH] = { 0 };
    int argumentCount = 1;

    // get first 
    char *saveptr;
    char *token = strtok_r(input, " ", &saveptr);
    cmd.command = token;

    while (token = strtok_r(NULL, " ", &saveptr)) {
        if (argumentCount == 1) {
            arg.argument = token;
            argumentCount++;
        } else {
            if (strcmp("<", token) == 0) {
                // process stdin redirect 
                char *fileName = strtok_r(NULL, " ", &saveptr);
                cmd.inputFile = fileName;
            } else if (strcmp(">", token) == 0) {
                // process stdout redirect 
                char *fileName = strtok_r(NULL, " ", &saveptr);
                cmd.outputFile = fileName;
            } else if (strcmp("&\n", token) == 0){
                // process run in background 
                printf("ampersand!");
                cmd.background = true;
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
    cmd.arguments = &arg; 

    return cmd;
}

void expandInput(char input[], char newInput[]) {
    pid_t pid = getpid();

    char pidStr[4] = "123\0";

    bool firstSymbol = false;
    
    // loop through old input look for $
    for (int i = 0; i < strlen(input); i++) {
        // found $
        if (input[i] == '$') {
            // check if second $ and if so expand 
            if (firstSymbol) {
                strcat(newInput, pidStr);
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
                strcat(newInput, "$");
            }
            // add input to newinput 
            char toCat[2] = { input[i], '\0' };
            strcat(newInput, toCat);
        }
    }
}

int main(void) {
    while (1) {
        char userInput[MAX_LENGTH];
        printf(": ");
        fgets(userInput, MAX_LENGTH, stdin);

        // if comment or blank line ignore all input
        if (userInput[0] != '#' && strcmp(userInput,"\n") != 0) {
            char expandedInput[MAX_LENGTH];
            expandInput(userInput, expandedInput);
            printf("%s", expandedInput);
            // struct command input = parseInput(userInput);
            // struct argument *args = input.arguments;
            fflush(stdin);
            fflush(stdout);   
        }
    }
    return 0;
}